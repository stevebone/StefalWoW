# TrinityCore Playerbot Architecture

**Version:** WoW 11.2 Compatible
**Target:** 5000 concurrent bots
**Module Location:** `src/modules/Playerbot/`

---

## Table of Contents

1. [System Overview](#system-overview)
2. [Module Structure](#module-structure)
3. [Core vs Module Separation](#core-vs-module-separation)
4. [Component Architecture](#component-architecture)
5. [Data Flow](#data-flow)
6. [Thread Model](#thread-model)
7. [Memory Management](#memory-management)
8. [Design Patterns](#design-patterns)
9. [Performance Architecture](#performance-architecture)
10. [Scalability](#scalability)

---

## System Overview

### High-Level Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        TrinityCore Server                        │
│                                                                   │
│  ┌────────────┐  ┌────────────┐  ┌─────────────────────────┐   │
│  │  bnetserver│  │ worldserver│  │  MySQL 9.4 Database    │   │
│  │ (unchanged)│  │            │  │  ┌──────┬──────┬──────┐ │   │
│  └────────────┘  │ ┌────────┐ │  │  │ auth │ char │ world│ │   │
│                  │ │  Core  │ │  │  └──────┴──────┴──────┘ │   │
│                  │ │Systems │ │  │                          │   │
│                  │ └────┬───┘ │  └─────────────────────────┘   │
│                  │      │     │                                 │
│                  │      ▼     │                                 │
│                  │ ┌────────────────────────────────────┐      │
│                  │ │   Playerbot Module (Optional)      │      │
│                  │ │                                     │      │
│                  │ │  ┌────────────┬──────────────┐    │      │
│                  │ │  │ BotSpawner │ BotWorldMgr  │    │      │
│                  │ │  └─────┬──────┴──────┬───────┘    │      │
│                  │ │        │             │             │      │
│                  │ │  ┌─────▼─────┐ ┌────▼────────┐   │      │
│                  │ │  │BotSessions│ │  Bot AI     │   │      │
│                  │ │  │(Socketless)│ │  System    │   │      │
│                  │ │  └───────────┘ └─────────────┘   │      │
│                  │ └────────────────────────────────────┘      │
│                  └───────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
```

### Design Philosophy

1. **Module-First Approach**: All functionality in `src/modules/Playerbot/`
2. **Zero Core Modification**: TrinityCore compiles and runs with/without module
3. **Optional Compilation**: `-DBUILD_PLAYERBOT=1` flag enables module
4. **Backward Compatible**: No breaking changes to existing TrinityCore APIs
5. **Performance First**: Designed for 5000 concurrent bots from ground up

---

## Module Structure

### Directory Organization

```
src/modules/Playerbot/
├── Account/                    # Bot account management
│   ├── BotAccountMgr.h/cpp    # Account pool manager
│   └── BotCharacterDistribution.h/cpp
│
├── AI/                         # AI decision making
│   ├── BotAI.h/cpp            # Base AI controller
│   ├── BotAIFactory.cpp       # AI factory
│   ├── Actions/               # Action system
│   │   ├── Action.h
│   │   ├── CommonActions.h/cpp
│   │   └── SpellInterruptAction.h/cpp
│   ├── ClassAI/               # Class-specific AI
│   │   ├── ClassAI.h/cpp      # Base class AI
│   │   ├── Warriors/          # Warrior AI (3 specs)
│   │   ├── Paladins/          # Paladin AI (3 specs)
│   │   ├── Hunters/           # Hunter AI (3 specs)
│   │   ├── Rogues/            # Rogue AI (3 specs)
│   │   ├── Priests/           # Priest AI (3 specs)
│   │   ├── DeathKnights/      # DK AI (3 specs)
│   │   ├── Shamans/           # Shaman AI (3 specs)
│   │   ├── Mages/             # Mage AI (3 specs)
│   │   ├── Warlocks/          # Warlock AI (3 specs)
│   │   ├── Monks/             # Monk AI (3 specs)
│   │   ├── Druids/            # Druid AI (4 specs)
│   │   ├── DemonHunters/      # DH AI (2 specs)
│   │   └── Evokers/           # Evoker AI (3 specs)
│   ├── Combat/                # Combat systems
│   │   ├── InterruptCoordinator.h/cpp
│   │   ├── ThreatManager.h/cpp
│   │   ├── TargetSelector.h/cpp
│   │   └── PositionManager.h/cpp
│   ├── Learning/              # Machine learning adaptation
│   │   ├── BehaviorAdaptation.h/cpp
│   │   └── PlayerPatternRecognition.h/cpp
│   ├── Strategy/              # Strategy pattern system
│   │   ├── Strategy.h/cpp
│   │   └── GroupCombatStrategy.h/cpp
│   ├── Triggers/              # Trigger system
│   │   └── Trigger.h/cpp
│   └── Values/                # Value cache
│       └── Value.h/cpp
│
├── Commands/                  # Chat command handlers
│   └── PlayerbotCommands.cpp
│
├── Config/                    # Configuration system
│   ├── PlayerbotConfig.h/cpp # Config manager
│   └── PlayerbotLog.h        # Logging macros
│
├── Database/                  # Database integration
│   ├── PlayerbotDatabase.h   # Database connection
│   ├── PlayerbotCharacterDBInterface.h/cpp
│   ├── BotDatabasePool.h/cpp # Connection pooling
│   └── PlayerbotDatabaseStatements.h
│
├── Dungeon/                   # Dungeon automation
│   ├── DungeonBehavior.h/cpp
│   └── EncounterStrategy.h/cpp
│
├── Group/                     # Group coordination
│   ├── GroupInvitationHandler.h/cpp
│   ├── GroupCoordination.h/cpp
│   ├── GroupFormation.h/cpp
│   └── RoleOptimizer.h/cpp
│
├── Lifecycle/                 # Bot lifecycle
│   ├── BotSpawner.h/cpp      # Main spawner (5000 bot capacity)
│   ├── BotScheduler.h/cpp    # Update scheduling
│   ├── BotLifecycleMgr.h/cpp
│   ├── BotCharacterCreator.h/cpp
│   └── SpawnRequest.h        # Spawn request structure
│
├── Movement/                  # Movement behaviors
│   ├── LeaderFollowBehavior.h/cpp
│   ├── FormationManager.h/cpp
│   └── PathfindingManager.h/cpp
│
├── Performance/               # Performance monitoring
│   ├── BotPerformanceMonitor.h/cpp
│   ├── BotProfiler.h/cpp
│   ├── BotMemoryManager.h/cpp
│   └── MLPerformanceTracker.h/cpp
│
├── Quest/                     # Quest automation
│   ├── QuestPickup.h/cpp
│   ├── QuestCompletion.h/cpp
│   └── QuestAutomation.h/cpp
│
├── Session/                   # Session management
│   ├── BotSession.h/cpp      # Socketless session
│   ├── BotSessionMgr.h/cpp   # Legacy session manager
│   ├── BotWorldSessionMgr.h/cpp  # Native session manager
│   └── BotLoginQueryHolder.h
│
├── Social/                    # Social features
│   ├── AuctionAutomation.h/cpp
│   ├── TradeAutomation.h/cpp
│   ├── GuildIntegration.h/cpp
│   └── VendorInteraction.h/cpp
│
├── Scripts/                   # World script integration
│   └── PlayerbotWorldScript.cpp
│
└── CMakeLists.txt            # Module build configuration
```

### Key Files by Function

**Bot Creation & Spawning:**
- `BotSpawner.cpp` - Central spawner (1494 lines)
- `BotCharacterCreator.cpp` - Character creation
- `BotAccountMgr.cpp` - Account management

**Session Management:**
- `BotSession.cpp` - Socketless session (1172 lines)
- `BotWorldSessionMgr.cpp` - Native TrinityCore integration

**AI System:**
- `BotAI.cpp` - Base AI logic (831 lines)
- `ClassAI.cpp` - Class-specific base
- Individual class files (e.g., `WarriorAI.cpp`)

**Configuration:**
- `PlayerbotConfig.cpp` - Config loading/validation (563 lines)
- `playerbots.conf` - Configuration file

---

## Core vs Module Separation

### Module-Only Implementation (Preferred)

**Zero Core Modifications:**
```cpp
// All code in src/modules/Playerbot/
namespace Playerbot {
    class BotSpawner { /* ... */ };
    class BotSession : public WorldSession { /* ... */ };
    class BotAI { /* ... */ };
}
```

**Integration via CMake:**
```cmake
# src/modules/Playerbot/CMakeLists.txt
if (BUILD_PLAYERBOT)
    add_library(playerbot STATIC
        # All module source files
    )
    target_link_libraries(game PUBLIC playerbot)
endif()
```

### Core Integration Points (Minimal)

**1. World Script Hook (Single Point of Entry)**

```cpp
// src/modules/Playerbot/scripts/PlayerbotWorldScript.cpp
class PlayerBotWorldScript : public WorldScript {
public:
    void OnStartup() override {
        // Initialize playerbot systems
        sBotSpawner->Initialize();
    }

    void OnUpdate(uint32 diff) override {
        // Update bot systems
        sBotWorldSessionMgr->Update(diff);
    }
};
```

**Registration:**
```cpp
void AddSC_PlayerBotScripts() {
    new PlayerBotWorldScript();
}
```

**2. Session Creation Hook**

```cpp
// BotWorldSessionMgr creates WorldSession instances
// Uses standard TrinityCore session manager APIs
bool AddPlayerBot(ObjectGuid characterGuid, uint32 accountId) {
    auto session = std::make_shared<BotSession>(accountId);
    // Add to WorldSessionFilter (standard TrinityCore API)
    return true;
}
```

**3. Database Extensions (Optional)**

```sql
-- Separate playerbot database or tables in characters DB
CREATE TABLE IF NOT EXISTS `playerbot_accounts` (
    `account_id` INT UNSIGNED NOT NULL,
    `max_bots` INT UNSIGNED DEFAULT 10,
    PRIMARY KEY (`account_id`)
);
```

### Compilation Flags

**Enable Module:**
```bash
cmake .. -DBUILD_PLAYERBOT=1
```

**Disable Module (Default):**
```bash
cmake ..  # Module not compiled
```

---

## Component Architecture

### BotSpawner Architecture

```
┌─────────────────────────────────────────────────────────┐
│                     BotSpawner                          │
│                     (Singleton)                         │
│                                                          │
│  ┌─────────────────┐  ┌──────────────────┐            │
│  │  Configuration  │  │  Spawn Queue     │            │
│  │  - Max bots     │  │  - Async queue   │            │
│  │  - Batch size   │  │  - Lock-free     │            │
│  │  - Ratios       │  │  - Priority      │            │
│  └─────────────────┘  └──────────────────┘            │
│                                                          │
│  ┌─────────────────────────────────────────────────┐  │
│  │         Bot Tracking (Thread-Safe)              │  │
│  │  ┌──────────────┬────────────────────────┐     │  │
│  │  │_activeBots   │ ObjectGuid → ZoneID    │     │  │
│  │  │_botsByZone   │ ZoneID → [ObjectGuid]  │     │  │
│  │  │_activeBotCount│ atomic<uint32>        │     │  │
│  │  └──────────────┴────────────────────────┘     │  │
│  └─────────────────────────────────────────────────┘  │
│                                                          │
│  ┌─────────────────────────────────────────────────┐  │
│  │      Population Management                       │  │
│  │  - Zone population tracking                     │  │
│  │  - Dynamic spawn calculations                    │  │
│  │  - Bot-to-player ratio enforcement              │  │
│  └─────────────────────────────────────────────────┘  │
│                                                          │
│  ┌─────────────────────────────────────────────────┐  │
│  │      Character Management                        │  │
│  │  - BotAccountMgr integration                    │  │
│  │  - BotCharacterCreator                          │  │
│  │  - Character GUID allocation                     │  │
│  └─────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────┘
           │                      │
           ▼                      ▼
    ┌─────────────┐      ┌───────────────┐
    │BotWorldMgr  │      │ BotSession    │
    │(Native TC)  │      │ Factory       │
    └─────────────┘      └───────────────┘
```

### BotSession Architecture

```
┌──────────────────────────────────────────────────────┐
│              BotSession (Socketless)                 │
│              extends WorldSession                     │
│                                                       │
│  ┌─────────────────────────────────────────────┐   │
│  │  Constructor (No Socket)                     │   │
│  │  - nullptr socket parameter                  │   │
│  │  - Overrides PlayerDisconnected() → false   │   │
│  │  - No network packet processing              │   │
│  └─────────────────────────────────────────────┘   │
│                                                       │
│  ┌─────────────────────────────────────────────┐   │
│  │  Packet Queues (Simulated Network)          │   │
│  │  ┌──────────────┬──────────────────┐       │   │
│  │  │_incomingPkts │ queue<WorldPkt>  │       │   │
│  │  │_outgoingPkts │ queue<WorldPkt>  │       │   │
│  │  └──────────────┴──────────────────┘       │   │
│  │  Mutex: _packetMutex (recursive_timed)     │   │
│  └─────────────────────────────────────────────┘   │
│                                                       │
│  ┌─────────────────────────────────────────────┐   │
│  │  Login System (Async Query Holder)          │   │
│  │  - BotLoginQueryHolder (58 queries)         │   │
│  │  - HandleBotPlayerLogin() callback          │   │
│  │  - Atomic state: LOGIN_IN_PROGRESS          │   │
│  └─────────────────────────────────────────────┘   │
│                                                       │
│  ┌─────────────────────────────────────────────┐   │
│  │  AI Integration                              │   │
│  │  - BotAI* _ai (owned pointer)               │   │
│  │  - Created after successful login           │   │
│  │  - Updated every frame                       │   │
│  └─────────────────────────────────────────────┘   │
│                                                       │
│  ┌─────────────────────────────────────────────┐   │
│  │  State Management (Thread-Safe)             │   │
│  │  - atomic<bool> _active                     │   │
│  │  - atomic<bool> _destroyed                  │   │
│  │  - atomic<LoginState> _loginState           │   │
│  └─────────────────────────────────────────────┘   │
└──────────────────────────────────────────────────────┘
```

### BotAI Architecture (Strategy Pattern)

```
┌──────────────────────────────────────────────────────────┐
│                    BotAI (Base)                          │
│                                                           │
│  ┌────────────────────────────────────────────────────┐ │
│  │  UpdateAI(diff) - SINGLE ENTRY POINT               │ │
│  │  ┌──────────────────────────────────────────────┐ │ │
│  │  │ 1. UpdateValues(diff)                        │ │ │
│  │  │ 2. UpdateStrategies(diff) ←─ EVERY FRAME    │ │ │
│  │  │ 3. ProcessTriggers()                         │ │ │
│  │  │ 4. UpdateActions(diff)                       │ │ │
│  │  │ 5. UpdateMovement(diff) ←─ EVERY FRAME      │ │ │
│  │  │ 6. UpdateCombatState(diff)                   │ │ │
│  │  │ 7. if (InCombat) OnCombatUpdate(diff)       │ │ │
│  │  │ 8. UpdateIdleBehaviors(diff)                 │ │ │
│  │  └──────────────────────────────────────────────┘ │ │
│  └────────────────────────────────────────────────────┘ │
│                                                           │
│  ┌────────────────────────────────────────────────────┐ │
│  │  Strategy System                                   │ │
│  │  ┌──────────────────────────────────────────────┐ │ │
│  │  │ _strategies: map<string, unique_ptr<Strategy>>│ │ │
│  │  │ _activeStrategies: vector<string>            │ │ │
│  │  │                                               │ │ │
│  │  │ Default Strategies:                           │ │ │
│  │  │  - "follow" (LeaderFollowBehavior)          │ │ │
│  │  │  - "group_combat" (GroupCombatStrategy)     │ │ │
│  │  │                                               │ │ │
│  │  │ Activated when bot joins group               │ │ │
│  │  └──────────────────────────────────────────────┘ │ │
│  └────────────────────────────────────────────────────┘ │
│                                                           │
│  ┌────────────────────────────────────────────────────┐ │
│  │  Action System (Command Pattern)                  │ │
│  │  ┌──────────────────────────────────────────────┐ │ │
│  │  │ _actionQueue: queue<Action, Context>         │ │ │
│  │  │ _currentAction: shared_ptr<Action>           │ │ │
│  │  │                                               │ │ │
│  │  │ Execution Priority:                           │ │ │
│  │  │  1. Triggered actions (from triggers)        │ │ │
│  │  │  2. Queued actions (explicit requests)       │ │ │
│  │  └──────────────────────────────────────────────┘ │ │
│  └────────────────────────────────────────────────────┘ │
│                                                           │
│  ┌────────────────────────────────────────────────────┐ │
│  │  Trigger System                                    │ │
│  │  ┌──────────────────────────────────────────────┐ │ │
│  │  │ _triggers: vector<shared_ptr<Trigger>>       │ │ │
│  │  │ _triggeredActions: priority_queue<Result>    │ │ │
│  │  │                                               │ │ │
│  │  │ Each frame:                                   │ │ │
│  │  │  1. Check() - Fast pre-check                 │ │ │
│  │  │  2. Evaluate() - Full evaluation             │ │ │
│  │  │  3. Queue action with urgency                │ │ │
│  │  └──────────────────────────────────────────────┘ │ │
│  └────────────────────────────────────────────────────┘ │
└──────────────────────────────────────────────────────────┘
                         │
                         ▼
          ┌──────────────────────────────┐
          │    ClassAI (Derived)         │
          │                              │
          │  OnCombatUpdate(diff)       │
          │  - UpdateRotation()          │
          │  - UpdateBuffs()             │
          │  - UpdateCooldowns()         │
          │                              │
          │  NO base UpdateAI override!  │
          │  NO movement control!        │
          └──────────────────────────────┘
```

### Group Coordination Architecture

```
┌────────────────────────────────────────────────────┐
│          Group Invitation Flow                     │
│                                                     │
│  Player sends invite                               │
│       │                                            │
│       ▼                                            │
│  ┌──────────────────────────────────────────┐    │
│  │ SMSG_PARTY_INVITE packet                 │    │
│  └──────────────────────────────────────────┘    │
│       │                                            │
│       ▼                                            │
│  ┌──────────────────────────────────────────┐    │
│  │ BotSession::SendPacket() intercepts      │    │
│  │ Calls HandleGroupInvitation()            │    │
│  └──────────────────────────────────────────┘    │
│       │                                            │
│       ▼                                            │
│  ┌──────────────────────────────────────────┐    │
│  │ Parse packet (inviter, roles, etc.)      │    │
│  │ Call Group::AddInvite(bot)               │    │
│  │ Sets bot->SetGroupInvite()               │    │
│  └──────────────────────────────────────────┘    │
│       │                                            │
│       ▼                                            │
│  ┌──────────────────────────────────────────┐    │
│  │ GroupInvitationHandler::HandleInvitation()│   │
│  │ Queue invitation for processing           │    │
│  └──────────────────────────────────────────┘    │
│       │                                            │
│       ▼                                            │
│  ┌──────────────────────────────────────────┐    │
│  │ GroupInvitationHandler::Update()         │    │
│  │ Auto-accept after delay                   │    │
│  └──────────────────────────────────────────┘    │
│       │                                            │
│       ▼                                            │
│  ┌──────────────────────────────────────────┐    │
│  │ Group::AddMember(bot)                    │    │
│  │ Standard TrinityCore group joining       │    │
│  └──────────────────────────────────────────┘    │
│       │                                            │
│       ▼                                            │
│  ┌──────────────────────────────────────────┐    │
│  │ BotAI::OnGroupJoined()                   │    │
│  │ - ActivateStrategy("follow")             │    │
│  │ - ActivateStrategy("group_combat")       │    │
│  │ - SetAIState(FOLLOWING)                  │    │
│  └──────────────────────────────────────────┘    │
└────────────────────────────────────────────────────┘
```

---

## Data Flow

### Bot Spawn Flow

```
Player → .bot spawn command
  │
  ▼
PlayerbotCommands::HandlePlayerbotSpawnCommand()
  │
  ├─→ Validate input (class, race, gender, name)
  │
  ▼
BotSpawner::CreateAndSpawnBot()
  │
  ├─→ BotCharacterCreator::CreateBotCharacter()
  │   ├─→ Generate character GUID
  │   ├─→ Allocate unique name
  │   ├─→ Create Player object
  │   ├─→ Player::Create() (TrinityCore API)
  │   ├─→ SaveToDB() (async transaction)
  │   └─→ Return ObjectGuid
  │
  ├─→ BotWorldSessionMgr::AddPlayerBot()
  │   ├─→ Create BotSession
  │   ├─→ BotSession::LoginCharacter() (async)
  │   │   ├─→ Create BotLoginQueryHolder (58 queries)
  │   │   ├─→ CharacterDatabase.DelayQueryHolder()
  │   │   └─→ Callback: HandleBotPlayerLogin()
  │   │       ├─→ Create Player object
  │   │       ├─→ LoadFromDB(holder) (async)
  │   │       ├─→ AddPlayerToMap()
  │   │       ├─→ ObjectAccessor::AddObject()
  │   │       ├─→ SendInitialPackets()
  │   │       └─→ Create BotAI
  │   │           └─→ BotAIFactory::CreateAI()
  │   │               └─→ ClassAIFactory::CreateClassAI()
  │   │
  │   └─→ Add to WorldSessionFilter (TrinityCore)
  │
  └─→ Update spawn statistics
      ├─→ totalSpawned++
      ├─→ currentlyActive++
      └─→ peakConcurrent (if new peak)
```

### Bot Update Flow (Every Frame)

```
WorldServerUpdate (10ms tick)
  │
  ▼
PlayerbotWorldScript::OnUpdate(diff)
  │
  ▼
BotWorldSessionMgr::Update(diff)
  │
  ├─→ For each BotSession:
  │   │
  │   ├─→ BotSession::Update(diff, filter)
  │   │   │
  │   │   ├─→ ProcessPendingLogin() (if logging in)
  │   │   │
  │   │   ├─→ ProcessQueryCallbacks() (async DB callbacks)
  │   │   │
  │   │   ├─→ ProcessBotPackets() (packet queue)
  │   │   │   ├─→ Extract packets (lock-free batch)
  │   │   │   └─→ Process through WorldSession
  │   │   │
  │   │   └─→ if (AI && Player && IsInWorld):
  │   │       └─→ AI->UpdateAI(diff)
  │   │           │
  │   │           ├─→ UpdateValues(diff)
  │   │           │
  │   │           ├─→ UpdateStrategies(diff) ← EVERY FRAME
  │   │           │   └─→ For each active strategy:
  │   │           │       ├─→ IsActive() check
  │   │           │       └─→ UpdateBehavior(ai, diff)
  │   │           │           └─→ (e.g., LeaderFollowBehavior)
  │   │           │               ├─→ Find group leader
  │   │           │               ├─→ Calculate follow position
  │   │           │               └─→ MotionMaster::MoveFollow()
  │   │           │
  │   │           ├─→ ProcessTriggers()
  │   │           │   └─→ For each trigger:
  │   │           │       ├─→ Check() (fast)
  │   │           │       └─→ if true: Evaluate() (full)
  │   │           │           └─→ Queue action with urgency
  │   │           │
  │   │           ├─→ UpdateActions(diff)
  │   │           │   ├─→ Execute current action
  │   │           │   └─→ OR select next from queue
  │   │           │
  │   │           ├─→ UpdateMovement(diff) ← EVERY FRAME
  │   │           │
  │   │           ├─→ UpdateCombatState(diff)
  │   │           │   ├─→ Detect combat entry/exit
  │   │           │   └─→ Call OnCombatStart/End
  │   │           │
  │   │           ├─→ if (IsInCombat()):
  │   │           │   └─→ OnCombatUpdate(diff) [virtual]
  │   │           │       └─→ ClassAI override:
  │   │           │           ├─→ UpdateTargeting()
  │   │           │           ├─→ UpdateRotation(target)
  │   │           │           └─→ UpdateCooldowns(diff)
  │   │           │
  │   │           ├─→ GroupInvitationHandler::Update(diff)
  │   │           │
  │   │           └─→ if (!InCombat && !Following):
  │   │               └─→ UpdateIdleBehaviors(diff)
  │   │                   ├─→ QuestAutomation (5s throttle)
  │   │                   ├─→ TradeAutomation (10s throttle)
  │   │                   └─→ AuctionAutomation (30s throttle)
  │   │
  │   └─→ Record performance metrics
  │
  └─→ ProcessGlobalCallbacks()
```

---

## Thread Model

### Thread Safety Guarantees

**World Update Thread (Main Thread):**
- All BotSession updates
- All BotAI updates
- Strategy execution
- Action execution

**Async Thread Pool (TrinityCore):**
- Database queries (async query holders)
- Character loading
- Login query processing

**Lock-Free Operations:**
```cpp
// Atomic counters - safe to access from any thread
uint32 count = sBotSpawner->GetActiveBotCount(); // atomic load

// Statistics - atomic operations
_stats.totalSpawned.fetch_add(1);
_stats.currentlyActive.fetch_sub(1);
```

**Mutex-Protected Operations:**
```cpp
// Strategy management
std::shared_lock lock(_mutex);  // Read lock
std::unique_lock lock(_mutex);  // Write lock

// Packet queues
std::lock_guard<std::recursive_timed_mutex> lock(_packetMutex);

// Spawn queue
std::lock_guard<std::mutex> lock(_spawnQueueMutex);
```

### Deadlock Prevention

**Lock Ordering:**
1. `_zoneMutex` (BotSpawner)
2. `_botMutex` (BotSpawner)
3. `_packetMutex` (BotSession)
4. `_mutex` (BotAI)

**Lock-Free Batch Processing:**
```cpp
// Phase 1: Extract data with minimal lock time
{
    std::lock_guard lock(_mutex);
    // Quick copy of data
    dataCopy = _data;
} // Lock released

// Phase 2: Process without holding lock
ProcessData(dataCopy);

// Phase 3: Update results with minimal lock time
{
    std::lock_guard lock(_mutex);
    // Quick update
    _results = processed;
}
```

---

## Memory Management

### Memory Budget (Per Bot)

```
Total per bot: ~8-10 MB

Breakdown:
- Player object:        ~2 MB  (TrinityCore standard)
- BotSession:          ~1 MB  (packet queues, state)
- BotAI:               ~0.5 MB (strategies, actions, triggers)
- ClassAI:             ~0.5 MB (class-specific data)
- WorldSession data:   ~2 MB  (TrinityCore standard)
- Strategy data:       ~0.5 MB (follow, combat, etc.)
- Misc overhead:       ~1.5 MB (allocator overhead, cache)
```

### RAII Patterns

```cpp
// Unique pointers for ownership
std::unique_ptr<BotAI> _ai;
std::unique_ptr<Strategy> _strategy;
std::unique_ptr<Action> _action;

// Shared pointers for shared ownership
std::shared_ptr<BotSession> session;
std::shared_ptr<Action> action;

// Smart pointers prevent memory leaks
{
    auto session = BotSession::Create(accountId);
    // Automatic cleanup on scope exit
}
```

### Object Pooling (Future Optimization)

```cpp
// Planned for 5000 bot scalability
class ObjectPool<T> {
    std::vector<std::unique_ptr<T>> _pool;
    std::queue<T*> _available;

public:
    T* Acquire();
    void Release(T* obj);
};

// Usage
ObjectPool<WorldPacket> packetPool;
WorldPacket* packet = packetPool.Acquire();
// ... use packet
packetPool.Release(packet);
```

---

## Design Patterns

### 1. Singleton Pattern

**Used For:** Global managers

```cpp
class BotSpawner {
public:
    static BotSpawner* instance() {
        static BotSpawner instance;
        return &instance;
    }
private:
    BotSpawner() = default;
};

#define sBotSpawner BotSpawner::instance()
```

### 2. Factory Pattern

**Used For:** AI creation

```cpp
class BotAIFactory {
public:
    std::unique_ptr<BotAI> CreateAI(Player* bot) {
        uint8 classId = bot->GetClass();
        switch (classId) {
            case CLASS_WARRIOR:
                return std::make_unique<WarriorAI>(bot);
            case CLASS_PALADIN:
                return std::make_unique<PaladinAI>(bot);
            // ... other classes
        }
    }
};
```

### 3. Strategy Pattern

**Used For:** Behavior composition

```cpp
class Strategy {
public:
    virtual std::string GetName() const = 0;
    virtual bool IsActive(BotAI const* ai) const = 0;
    virtual void UpdateBehavior(BotAI* ai, uint32 diff) = 0;
};

class LeaderFollowBehavior : public Strategy {
public:
    std::string GetName() const override { return "follow"; }
    void UpdateBehavior(BotAI* ai, uint32 diff) override {
        // Follow logic
    }
};
```

### 4. Command Pattern

**Used For:** Action system

```cpp
class Action {
public:
    virtual bool IsPossible(BotAI* ai) = 0;
    virtual bool IsUseful(BotAI* ai) = 0;
    virtual ActionResult Execute(BotAI* ai, ActionContext const& ctx) = 0;
};

// Queue and execute actions
botAI->QueueAction(std::make_shared<HealAction>());
```

### 5. Observer Pattern

**Used For:** Event handling

```cpp
class Trigger {
public:
    virtual bool Check(BotAI const* ai) = 0;
    virtual TriggerResult Evaluate(BotAI* ai) = 0;
};

// Triggers observe bot state and suggest actions
class LowHealthTrigger : public Trigger {
    bool Check(BotAI const* ai) override {
        return ai->GetBot()->GetHealthPct() < 30.0f;
    }
};
```

### 6. Adapter Pattern

**Used For:** TrinityCore integration

```cpp
class BotSession : public WorldSession {
    // Adapts WorldSession for socketless operation
    bool PlayerDisconnected() const override { return false; }
    void SendPacket(WorldPacket const* packet, bool forced) override {
        // Queue packet instead of sending over network
    }
};
```

---

## Performance Architecture

### Scalability Design (5000 Bots)

**Current State (500-800 bots):**
- Linear scaling
- ~0.1% CPU per bot
- ~10 MB memory per bot
- Single-threaded update loop

**Target State (5000 bots):**
```
Performance Targets:
- CPU: <10% total for 5000 bots
- Memory: <50 GB total
- Update rate: 100ms per bot (10 Hz)
- Database: <1000 queries/sec
```

**Optimization Strategies:**

1. **Lock-Free Data Structures**
```cpp
// Atomic operations instead of mutex
std::atomic<uint32> _activeBotCount;
uint32 count = _activeBotCount.load(std::memory_order_acquire);
```

2. **Batch Processing**
```cpp
// Process bots in batches
constexpr size_t BATCH_SIZE = 100;
for (size_t i = 0; i < sessions.size(); i += BATCH_SIZE) {
    ProcessBatch(sessions, i, std::min(i + BATCH_SIZE, sessions.size()));
}
```

3. **Async Database Operations**
```cpp
// Non-blocking queries
sPlayerbotCharDB->ExecuteAsync(stmt, [](PreparedQueryResult result) {
    // Process on callback thread
});
```

4. **Object Pooling**
```cpp
// Reuse expensive objects
WorldPacket* packet = _packetPool->Acquire();
// Use packet
_packetPool->Release(packet);
```

5. **Throttled Updates**
```cpp
// Expensive operations run less frequently
if (currentTime - _lastExpensiveUpdate > 500) {
    UpdateBuffs(); // Only every 500ms
    _lastExpensiveUpdate = currentTime;
}
```

### Performance Monitoring

**Metrics Tracked:**
- Update time per bot (µs)
- Memory usage per bot (bytes)
- Database query time (ms)
- AI decision time (µs)
- Spawn/despawn time (µs)

**Access:**
```cpp
SpawnStats const& stats = sBotSpawner->GetStats();
TC_LOG_INFO("module.playerbot", "Avg spawn time: {:.2f}ms", stats.GetAverageSpawnTime());
TC_LOG_INFO("module.playerbot", "Success rate: {:.2f}%", stats.GetSuccessRate());
```

---

## Scalability

### Current Capacity Analysis

**Bottlenecks (500-800 bots):**
1. Single-threaded update loop
2. Database query overhead
3. Memory allocation patterns
4. Packet processing overhead

**Observed Performance:**
- 500 bots: ~5% CPU, stable
- 800 bots: ~8% CPU, occasional lag spikes
- 1000 bots: ~12% CPU, noticeable lag

### Roadmap to 5000 Bots

**Phase 1: Multi-Threading (Target: 2000 bots)**
```cpp
// Thread pool for bot updates
ThreadPool<BotSession> updatePool(std::thread::hardware_concurrency());

void Update(uint32 diff) {
    for (auto& session : _sessions) {
        updatePool.QueueTask([session, diff]() {
            session->Update(diff);
        });
    }
    updatePool.WaitAll();
}
```

**Phase 2: Advanced Caching (Target: 3500 bots)**
```cpp
// Shared spell info cache
class SpellInfoCache {
    static std::unordered_map<uint32, SpellInfo const*> _cache;
public:
    static SpellInfo const* Get(uint32 spellId);
};

// Reduce per-bot memory
```

**Phase 3: Load Balancing (Target: 5000 bots)**
```cpp
// Distribute bots across multiple world servers
class BotLoadBalancer {
    std::vector<WorldServer*> _servers;
public:
    WorldServer* SelectServer(uint32 zoneId);
    void RebalanceBots();
};
```

**Phase 4: Database Optimization (Target: 5000+ bots)**
```sql
-- Partitioned tables for better query performance
ALTER TABLE characters
PARTITION BY RANGE (account) (
    PARTITION p0 VALUES LESS THAN (10000),
    PARTITION p1 VALUES LESS THAN (20000),
    PARTITION p2 VALUES LESS THAN (30000)
);

-- Optimized indexes
CREATE INDEX idx_bot_sessions ON characters (account, online);
```

---

## Component Interaction Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                     TrinityCore World Server                     │
│                                                                   │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │                  Main Update Loop (10ms)                  │  │
│  └────────┬─────────────────────────────────────────────────┘  │
│           │                                                      │
│           ▼                                                      │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │            PlayerbotWorldScript::OnUpdate()              │  │
│  └────────┬─────────────────────────────────────────────────┘  │
│           │                                                      │
│           ▼                                                      │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │          BotWorldSessionMgr::Update(diff)                │  │
│  │  ┌────────────────────────────────────────────────────┐ │  │
│  │  │ For each BotSession:                               │ │  │
│  │  │   session->Update(diff, filter)                    │ │  │
│  │  └─────────┬──────────────────────────────────────────┘ │  │
│  └────────────┼──────────────────────────────────────────────┘  │
│               │                                                  │
│               ▼                                                  │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │             BotSession::Update(diff)                     │  │
│  │  ┌────────────────────────────────────────────────────┐ │  │
│  │  │ 1. ProcessQueryCallbacks() ─→ DB async results    │ │  │
│  │  │ 2. ProcessBotPackets() ─────→ Packet queue        │ │  │
│  │  │ 3. AI->UpdateAI(diff) ──────→ Main AI logic       │ │  │
│  │  └─────────┬──────────────────────────────────────────┘ │  │
│  └────────────┼──────────────────────────────────────────────┘  │
│               │                                                  │
│               ▼                                                  │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │              BotAI::UpdateAI(diff)                       │  │
│  │  ┌────────────────────────────────────────────────────┐ │  │
│  │  │ UpdateStrategies(diff) ───┐                        │ │  │
│  │  │                            ▼                        │ │  │
│  │  │                   ┌─────────────────┐              │ │  │
│  │  │                   │ LeaderFollow    │              │ │  │
│  │  │                   │ GroupCombat     │              │ │  │
│  │  │                   │ ... more ...    │              │ │  │
│  │  │                   └─────────────────┘              │ │  │
│  │  │                                                     │ │  │
│  │  │ ProcessTriggers() ────────┐                        │ │  │
│  │  │                            ▼                        │ │  │
│  │  │                   ┌─────────────────┐              │ │  │
│  │  │                   │ LowHealth       │              │ │  │
│  │  │                   │ EnemyNearby     │              │ │  │
│  │  │                   │ ... more ...    │              │ │  │
│  │  │                   └─────────────────┘              │ │  │
│  │  │                                                     │ │  │
│  │  │ if (InCombat):                                     │ │  │
│  │  │   OnCombatUpdate(diff) [virtual]                   │ │  │
│  │  │        │                                            │ │  │
│  │  │        ▼                                            │ │  │
│  │  │   ┌─────────────────────────────────────┐          │ │  │
│  │  │   │ ClassAI::OnCombatUpdate()           │          │ │  │
│  │  │   │  - UpdateRotation(target)           │          │ │  │
│  │  │   │  - UpdateCooldowns(diff)            │          │ │  │
│  │  │   │  - UpdateBuffs()                    │          │ │  │
│  │  │   └─────────────────────────────────────┘          │ │  │
│  │  └────────────────────────────────────────────────────┘ │  │
│  └──────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

---

## Conclusion

The TrinityCore Playerbot module demonstrates a clean, modular architecture that:

1. **Separates Concerns**: Module-only implementation with minimal core touchpoints
2. **Scales Efficiently**: Designed from ground up for 5000 concurrent bots
3. **Follows Patterns**: Uses proven design patterns (Strategy, Factory, Command, Observer)
4. **Maintains Quality**: Thread-safe, memory-efficient, performance-monitored
5. **Stays Compatible**: Zero breaking changes to existing TrinityCore functionality

The architecture is production-ready for 500-800 bots and has a clear roadmap to 5000+ bots through multi-threading, caching, and load balancing optimizations.
