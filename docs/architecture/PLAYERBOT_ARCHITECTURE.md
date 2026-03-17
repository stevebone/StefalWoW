# TrinityCore PlayerBot Architecture

**Version**: 1.0
**Last Updated**: 2025-10-04
**Target Audience**: Architects, Senior Developers

---

## Table of Contents

1. [System Overview](#system-overview)
2. [Component Architecture](#component-architecture)
3. [Data Flow](#data-flow)
4. [Class Hierarchy](#class-hierarchy)
5. [Threading Model](#threading-model)
6. [Performance Architecture](#performance-architecture)
7. [Integration Points](#integration-points)

---

## System Overview

### High-Level Architecture

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         TrinityCore WorldServer                              │
│                                                                               │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                    PlayerBot Module (Optional)                       │   │
│  │                                                                       │   │
│  │  ┌────────────────┐  ┌────────────────┐  ┌────────────────┐        │   │
│  │  │   Phase 1-2    │  │   Phase 3-4    │  │    Phase 5     │        │   │
│  │  │  Core & Infra  │  │  Game Systems  │  │  Performance   │        │   │
│  │  │                │  │                │  │                │        │   │
│  │  │ • BotSession   │  │ • Combat AI    │  │ • ThreadPool   │        │   │
│  │  │ • BotScheduler │  │ • Movement     │  │ • MemoryPool   │        │   │
│  │  │ • BotAccount   │  │ • Quest System │  │ • QueryOptim   │        │   │
│  │  │ • Config       │  │ • Social       │  │ • Profiler     │        │   │
│  │  └────────────────┘  └────────────────┘  └────────────────┘        │   │
│  │                                                                       │   │
│  │  ┌─────────────────────────────────────────────────────────────┐   │   │
│  │  │              BotAI Framework (13 Class AI Systems)           │   │   │
│  │  │                                                               │   │   │
│  │  │  Death Knight │ Demon Hunter │ Druid │ Evoker │ Hunter     │   │   │
│  │  │  Mage │ Monk │ Paladin │ Priest │ Rogue │ Shaman │ Warlock │   │   │
│  │  │  Warrior │                                                    │   │   │
│  │  └─────────────────────────────────────────────────────────────┘   │   │
│  └───────────────────────────────────────────────────────────────────┘   │
│                                                                               │
│  ┌───────────────────────────────────────────────────────────────────┐     │
│  │              TrinityCore Game Systems (Unchanged)                  │     │
│  │                                                                     │     │
│  │  World Management │ Map System │ Spell System │ Combat System     │     │
│  │  Database Layer │ Network Layer │ Script System │ Event System    │     │
│  └───────────────────────────────────────────────────────────────────┘     │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    │
                        ┌───────────┴───────────┐
                        │   MySQL Database      │
                        │                       │
                        │ • playerbot_auth      │
                        │ • playerbot_characters│
                        │ • playerbot_world     │
                        └───────────────────────┘
```

### Design Principles

1. **Module Isolation**: 100% of bot code in `src/modules/Playerbot/`
2. **Minimal Core Integration**: Hook/observer pattern for TrinityCore events
3. **Performance First**: <0.1% CPU per bot, <10MB memory per bot
4. **Scalability**: Support 1-5000 concurrent bots
5. **Thread Safety**: Lock-free where possible, careful synchronization everywhere else
6. **Optional Compilation**: Can be compiled out entirely (`-DBUILD_PLAYERBOT=0`)

---

## Component Architecture

### Phase 1-2: Core Bot Framework & Infrastructure

```
┌─────────────────────────────────────────────────────────────────────┐
│                    Core Bot Framework (Phase 1-2)                    │
├─────────────────────────────────────────────────────────────────────┤
│                                                                       │
│  ┌──────────────────┐        ┌──────────────────┐                  │
│  │  BotAccountMgr   │        │  BotSessionMgr   │                  │
│  │                  │        │                  │                  │
│  │ • CreateAccount  │───────▶│ • CreateSession  │                  │
│  │ • DeleteAccount  │        │ • DestroySession │                  │
│  │ • ListBots       │        │ • GetSession     │                  │
│  └──────────────────┘        └──────────────────┘                  │
│           │                           │                              │
│           │                           │                              │
│           ▼                           ▼                              │
│  ┌──────────────────┐        ┌──────────────────┐                  │
│  │  BotCharacter    │        │   BotSession     │                  │
│  │                  │        │                  │                  │
│  │ • Guid           │◀───────│ • _player        │                  │
│  │ • Name           │        │ • _botAI         │                  │
│  │ • Class/Race     │        │ • Update()       │                  │
│  │ • Level          │        │ • HandlePacket() │                  │
│  └──────────────────┘        └──────────────────┘                  │
│                                       │                              │
│                                       │ Creates                      │
│                                       ▼                              │
│                              ┌──────────────────┐                   │
│                              │     BotAI        │                   │
│                              │                  │                   │
│                              │ • UpdateAI()     │                   │
│                              │ • HandleEvent()  │                   │
│                              │ • ExecuteAction()│                   │
│                              └──────────────────┘                   │
│                                       │                              │
│                                       │ Delegates to                 │
│                                       ▼                              │
│                              ┌──────────────────┐                   │
│                              │    ClassAI       │                   │
│                              │                  │                   │
│                              │ • ExecuteRotation│                   │
│                              │ • HandleCombat() │                   │
│                              │ • UseAbilities() │                   │
│                              └──────────────────┘                   │
│                                                                       │
│  ┌──────────────────────────────────────────────────────────────┐  │
│  │              BotScheduler (Update Coordinator)                │  │
│  │                                                                │  │
│  │  Update(diff) {                                                │  │
│  │    for (bot : activeBots) {                                    │  │
│  │      ThreadPool::Submit([bot, diff]() {                        │  │
│  │        bot->GetBotAI()->UpdateAI(diff);                        │  │
│  │      });                                                         │  │
│  │    }                                                             │  │
│  │  }                                                               │  │
│  └──────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────┘
```

**Key Components**:

- **BotAccountMgr**: Account creation/deletion with 10-character limit
- **BotSessionMgr**: Session lifecycle (no network sockets)
- **BotCharacter**: Character data persistence
- **BotSession**: Lightweight session without network overhead
- **BotScheduler**: Coordinates bot updates across ThreadPool

### Phase 3-4: Game System Integration & Advanced Features

```
┌─────────────────────────────────────────────────────────────────────┐
│              Game System Integration (Phase 3-4)                     │
├─────────────────────────────────────────────────────────────────────┤
│                                                                       │
│  ┌──────────────────┐  ┌──────────────────┐  ┌─────────────────┐  │
│  │   Combat AI      │  │  Movement System │  │  Quest System   │  │
│  │                  │  │                  │  │                 │  │
│  │ • Threat Mgmt    │  │ • Pathfinding    │  │ • Pickup        │  │
│  │ • Target Select  │  │ • Follow Leader  │  │ • Progress      │  │
│  │ • Interrupts     │  │ • Positioning    │  │ • Complete      │  │
│  │ • Cooldowns      │  │ • Avoidance      │  │ • Turnin        │  │
│  └──────────────────┘  └──────────────────┘  └─────────────────┘  │
│           │                      │                      │            │
│           └──────────────────────┼──────────────────────┘            │
│                                  │                                   │
│                                  ▼                                   │
│                         ┌──────────────────┐                        │
│                         │     BotAI        │                        │
│                         │   (Coordinator)  │                        │
│                         └──────────────────┘                        │
│                                  │                                   │
│           ┌──────────────────────┼──────────────────────┐            │
│           │                      │                      │            │
│           ▼                      ▼                      ▼            │
│  ┌──────────────────┐  ┌──────────────────┐  ┌─────────────────┐  │
│  │ GroupCoordinator │  │ EconomyManager   │  │ SocialManager   │  │
│  │                  │  │                  │  │                 │  │
│  │ • Role Assign    │  │ • AH Trading     │  │ • Chat          │  │
│  │ • Loot Rules     │  │ • Profession     │  │ • Emotes        │  │
│  │ • Raid Tactics   │  │ • Gold Mgmt      │  │ • Guild         │  │
│  └──────────────────┘  └──────────────────┘  └─────────────────┘  │
│                                                                       │
│  ┌──────────────────────────────────────────────────────────────┐  │
│  │            Advanced Behavior Manager (ML-Ready)               │  │
│  │                                                                │  │
│  │  • Adaptive Difficulty                                         │  │
│  │  • Player Pattern Learning                                     │  │
│  │  • Behavioral Variation                                        │  │
│  └──────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────┘
```

**Key Components**:

- **Combat AI**: Threat management, target selection, interrupt coordination
- **Movement System**: Pathfinding, leader following, tactical positioning
- **Quest System**: Automatic quest pickup, progress, and turnin
- **GroupCoordinator**: Group formation, role assignment, loot rules
- **EconomyManager**: Auction house, profession crafting, gold management
- **SocialManager**: Chat responses, emotes, guild integration

### Phase 5: Performance Optimization

```
┌─────────────────────────────────────────────────────────────────────┐
│              Performance Optimization (Phase 5)                      │
├─────────────────────────────────────────────────────────────────────┤
│                                                                       │
│                    ┌─────────────────────────┐                      │
│                    │  PerformanceManager     │                      │
│                    │   (Central Coordinator) │                      │
│                    └───────────┬─────────────┘                      │
│                                │                                     │
│        ┌───────────────────────┼───────────────────────┐            │
│        │                       │                       │            │
│        ▼                       ▼                       ▼            │
│  ┌──────────┐         ┌──────────────┐       ┌──────────────┐     │
│  │ThreadPool│         │  MemoryPool  │       │QueryOptimizer│     │
│  │          │         │              │       │              │     │
│  │┌────────┐│         │┌────────────┐│       │┌────────────┐│     │
│  ││Priority││         ││Thread-Local││       ││Statement   ││     │
│  ││Queues  ││         ││Cache (32)  ││       ││Cache (LRU) ││     │
│  ││        ││         ││            ││       ││            ││     │
│  ││CRITICAL││         ││Fast Alloc  ││       ││Batch Ops   ││     │
│  ││HIGH    ││         ││<100ns      ││       ││Metrics     ││     │
│  ││NORMAL  ││         ││            ││       ││            ││     │
│  ││LOW     ││         ││Global Pool ││       ││Connection  ││     │
│  ││IDLE    ││         ││(Fallback)  ││       ││Pool        ││     │
│  │└────────┘│         │└────────────┘│       │└────────────┘│     │
│  │          │         │              │       │              │     │
│  │Work-     │         │Per-Bot Track │       │Slow Query   │     │
│  │Stealing  │         │Memory Limit  │       │Detection    │     │
│  │Queue     │         │Defrag        │       │>50ms        │     │
│  └──────────┘         └──────────────┘       └──────────────┘     │
│                                                                       │
│                    ┌─────────────────────────┐                      │
│                    │      Profiler           │                      │
│                    │                         │                      │
│                    │  ┌──────────────────┐  │                      │
│                    │  │ ScopedTimer      │  │                      │
│                    │  │ (RAII Pattern)   │  │                      │
│                    │  │                  │  │                      │
│                    │  │ • CPU Time       │  │                      │
│                    │  │ • Min/Max/Avg    │  │                      │
│                    │  │ • Call Count     │  │                      │
│                    │  │ • Sampling       │  │                      │
│                    │  └──────────────────┘  │                      │
│                    │                         │                      │
│                    │  Zero Overhead When     │                      │
│                    │  Disabled               │                      │
│                    └─────────────────────────┘                      │
└─────────────────────────────────────────────────────────────────────┘
```

**Performance Targets**:
- ThreadPool: <1μs task submission, >95% CPU utilization
- MemoryPool: <100ns allocation (cache hit), >95% hit rate
- QueryOptimizer: >90% cache hit, <50ms latency
- Profiler: <1% overhead when enabled, 0% when disabled

---

## Data Flow

### Bot Update Cycle

```
┌─────────────────────────────────────────────────────────────────────┐
│                      Bot Update Data Flow                            │
└─────────────────────────────────────────────────────────────────────┘

  WorldServer Update Loop (every ~100ms)
           │
           ▼
  ┌──────────────────┐
  │  BotScheduler    │
  │  ::Update(diff)  │
  └──────────────────┘
           │
           │ For each active bot
           ▼
  ┌──────────────────────────────────────┐
  │ ThreadPool::Submit(NORMAL, [bot]() { │
  │   bot->GetBotAI()->UpdateAI(diff);   │
  │ })                                    │
  └──────────────────────────────────────┘
           │
           │ Async execution in worker thread
           ▼
  ┌──────────────────┐
  │ BotAI::UpdateAI()│
  └──────────────────┘
           │
           ├──────────────┬──────────────┬──────────────┐
           ▼              ▼              ▼              ▼
  ┌──────────────┐ ┌──────────┐ ┌───────────┐ ┌──────────────┐
  │ Trigger      │ │ Action   │ │ Strategy  │ │ ClassAI      │
  │ Evaluation   │ │ Selection│ │ Execution │ │ Rotation     │
  └──────────────┘ └──────────┘ └───────────┘ └──────────────┘
           │              │              │              │
           └──────────────┴──────────────┴──────────────┘
                                │
                                ▼
                       ┌────────────────┐
                       │ Execute Actions│
                       │                │
                       │ • Cast Spell   │
                       │ • Move         │
                       │ • Interact     │
                       │ • Communicate  │
                       └────────────────┘
                                │
                                ▼
                    ┌──────────────────────┐
                    │ TrinityCore API Calls│
                    │                      │
                    │ • CastSpell()        │
                    │ • MoveToPosition()   │
                    │ • UpdatePosition()   │
                    │ • SendPacket()       │
                    └──────────────────────┘
```

### Combat Decision Flow

```
┌─────────────────────────────────────────────────────────────────────┐
│                   Combat Decision Data Flow                          │
└─────────────────────────────────────────────────────────────────────┘

  Enemy Engages Bot
         │
         ▼
  ┌─────────────────┐
  │ Event: Attacked │
  │ (From TC Core)  │
  └─────────────────┘
         │
         ▼
  ┌──────────────────────┐
  │ BotAI::HandleEvent() │
  │ (CRITICAL priority)  │
  └──────────────────────┘
         │
         ▼
  ┌────────────────────────────┐
  │ Combat State Evaluation    │
  │                            │
  │ • Threat level             │
  │ • Health/Mana              │
  │ • Cooldowns available      │
  │ • Group composition        │
  └────────────────────────────┘
         │
         ├────────────────┬────────────────┬────────────────┐
         ▼                ▼                ▼                ▼
  ┌──────────┐    ┌──────────┐    ┌──────────┐    ┌──────────┐
  │ Target   │    │ Position │    │ Interrupt│    │ Cooldown │
  │ Selection│    │ Manager  │    │ Coordin. │    │ Manager  │
  └──────────┘    └──────────┘    └──────────┘    └──────────┘
         │                │                │                │
         └────────────────┴────────────────┴────────────────┘
                                  │
                                  ▼
                         ┌────────────────┐
                         │ ClassAI        │
                         │ ExecuteRotation│
                         └────────────────┘
                                  │
                                  ▼
                    ┌──────────────────────────┐
                    │ Ability Priority Decision│
                    │                          │
                    │ 1. Defensive CDs (if low)│
                    │ 2. Interrupts (if casting│
                    │ 3. Burst CDs (if execute)│
                    │ 4. Rotation (normal)     │
                    └──────────────────────────┘
                                  │
                                  ▼
                         ┌────────────────┐
                         │ Cast Spell     │
                         │ (TrinityCore)  │
                         └────────────────┘
```

### Database Query Flow

```
┌─────────────────────────────────────────────────────────────────────┐
│                 Database Query Data Flow                             │
└─────────────────────────────────────────────────────────────────────┘

  Bot Needs Data (e.g., load character)
         │
         ▼
  ┌──────────────────────────────┐
  │ BotCharacter::LoadFromDB()   │
  └──────────────────────────────┘
         │
         ▼
  ┌──────────────────────────────┐
  │ QueryOptimizer::ExecuteQuery │
  └──────────────────────────────┘
         │
         ├─── Check cache ────┐
         │                    │
         ▼                    ▼
  ┌──────────────┐    ┌──────────────┐
  │ Cache Hit?   │    │ Cache Miss   │
  │ (>90% rate)  │    │              │
  └──────────────┘    └──────────────┘
         │                    │
         │ Yes                │ No
         │                    ▼
         │            ┌──────────────────┐
         │            │ Prepared Stmt    │
         │            │ From Cache (LRU) │
         │            └──────────────────┘
         │                    │
         │                    ▼
         │            ┌──────────────────┐
         │            │ Execute on       │
         │            │ Connection Pool  │
         │            └──────────────────┘
         │                    │
         │                    ▼
         │            ┌──────────────────┐
         │            │ Update Cache     │
         │            └──────────────────┘
         │                    │
         └────────────────────┘
                      │
                      ▼
              ┌──────────────┐
              │ Return Data  │
              └──────────────┘
```

---

## Class Hierarchy

### BotAI Class Hierarchy

```
                        ┌───────────┐
                        │   BotAI   │
                        │ (Abstract)│
                        └─────┬─────┘
                              │
              ┌───────────────┴───────────────┐
              │                               │
        ┌─────▼─────┐                   ┌────▼─────┐
        │  ClassAI  │                   │  Other   │
        │ (Abstract)│                   │Components│
        └─────┬─────┘                   └──────────┘
              │
              │ Specialized by class
              │
      ┌───────┴────────────────────────────────────────────┐
      │                                                     │
┌─────▼──────────┐                              ┌──────────▼─────────┐
│ DeathKnightAI  │                              │    WarriorAI       │
└─────┬──────────┘                              └──────────┬─────────┘
      │                                                     │
      │ Specialized by spec                                │
      │                                                     │
   ┌──┴──────────┬──────────────┬──────────────┐          │
   │             │              │              │       ┌───┴────┬─────────┐
┌──▼────┐  ┌────▼───┐  ┌──────▼─────┐  ┌─────▼──┐  │        │         │
│Blood  │  │Frost   │  │Unholy      │  │Deathbr.│  │Arms    │Fury    Prot
│Spec   │  │Spec    │  │Spec        │  │(Hero)  │  │Spec    │Spec    Spec
└───────┘  └────────┘  └────────────┘  └────────┘  └────────┴─────────┘

Similar hierarchies for:
- Demon Hunter (Havoc, Vengeance)
- Druid (Balance, Feral, Guardian, Restoration)
- Evoker (Devastation, Preservation, Augmentation)
- Hunter (Beast Mastery, Marksmanship, Survival)
- Mage (Arcane, Fire, Frost)
- Monk (Brewmaster, Mistweaver, Windwalker)
- Paladin (Holy, Protection, Retribution)
- Priest (Discipline, Holy, Shadow)
- Rogue (Assassination, Outlaw, Subtlety)
- Shaman (Elemental, Enhancement, Restoration)
- Warlock (Affliction, Demonology, Destruction)
```

### Component Relationships

```
┌──────────────────────────────────────────────────────────────────┐
│                    Component Relationships                        │
└──────────────────────────────────────────────────────────────────┘

  ┌────────────────┐
  │ BotScheduler   │───────┐
  └────────────────┘       │
                           │ Manages
                           ▼
                  ┌────────────────┐
                  │  BotSession    │───────┐
                  └────────────────┘       │
                           │               │ Contains
                           │ Owns          ▼
                           │      ┌────────────────┐
                           │      │  Player*       │
                           │      └────────────────┘
                           │               │
                           │               │ Has
                           ▼               ▼
                  ┌────────────────┐ ┌────────────────┐
                  │     BotAI      │ │  BotCharacter  │
                  └────────────────┘ └────────────────┘
                           │
                           │ Delegates to
                           │
        ┌──────────────────┼──────────────────┐
        │                  │                  │
        ▼                  ▼                  ▼
  ┌──────────┐    ┌──────────────┐    ┌──────────┐
  │ Movement │    │   ClassAI    │    │ Strategy │
  │ Manager  │    │  (Spec AI)   │    │ System   │
  └──────────┘    └──────────────┘    └──────────┘
        │                  │                  │
        │                  │                  │
        └──────────────────┼──────────────────┘
                           │
                           │ Uses
                           │
        ┌──────────────────┼──────────────────┐
        │                  │                  │
        ▼                  ▼                  ▼
  ┌──────────┐    ┌──────────────┐    ┌──────────┐
  │ Target   │    │  Interrupt   │    │ Cooldown │
  │ Selector │    │  Coordinator │    │ Manager  │
  └──────────┘    └──────────────┘    └──────────┘
```

---

## Threading Model

### Thread Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                        Threading Model                               │
└─────────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────┐
│                   WorldServer Main Thread                         │
│                                                                   │
│  • World update loop (100ms tick)                                │
│  • Map updates                                                   │
│  • Player updates                                                │
│  • BotScheduler::Update() ──────┐                               │
└──────────────────────────────────│───────────────────────────────┘
                                   │
                                   │ Enqueues bot updates
                                   │
                                   ▼
┌─────────────────────────────────────────────────────────────────────┐
│                         ThreadPool System                            │
│                                                                       │
│  ┌──────────────┐   ┌──────────────┐   ┌──────────────┐            │
│  │ Worker Thread│   │ Worker Thread│   │ Worker Thread│  ... (N)   │
│  │      #1      │   │      #2      │   │      #N      │            │
│  └──────┬───────┘   └──────┬───────┘   └──────┬───────┘            │
│         │                  │                  │                     │
│         │ Steal work       │ Steal work       │ Steal work          │
│         │                  │                  │                     │
│  ┌──────▼──────────────────▼──────────────────▼─────────────────┐  │
│  │            Priority Queues (Lock-Free Deques)                 │  │
│  │                                                                │  │
│  │  CRITICAL: [Task][Task]                    (Combat)           │  │
│  │  HIGH:     [Task][Task][Task]              (Movement)         │  │
│  │  NORMAL:   [Task][Task][Task][Task]        (AI Updates)       │  │
│  │  LOW:      [Task][Task]                    (Social)           │  │
│  │  IDLE:     [Task]                          (Background)       │  │
│  └────────────────────────────────────────────────────────────────┘  │
│                                                                       │
│  Work-Stealing Algorithm:                                            │
│  1. Each worker owns a local deque                                   │
│  2. Push/Pop from bottom (LIFO for cache locality)                   │
│  3. Other workers steal from top (FIFO for fairness)                 │
│  4. Exponential backoff on contention                                │
└─────────────────────────────────────────────────────────────────────┘
                                   │
                                   │ Execute tasks
                                   │
                                   ▼
┌─────────────────────────────────────────────────────────────────────┐
│                      Bot AI Updates (Async)                          │
│                                                                       │
│  • BotAI::UpdateAI() runs in worker thread                          │
│  • ClassAI rotation calculations                                     │
│  • Movement pathfinding                                              │
│  • Quest logic                                                       │
│  • All work is isolated per-bot (thread-safe by design)             │
└─────────────────────────────────────────────────────────────────────┘
```

### Thread Safety Guarantees

1. **BotSession**: Each bot session is independent, no shared mutable state
2. **BotAI**: Updated by single thread at a time (queued in ThreadPool)
3. **MemoryPool**: Thread-local caching eliminates contention
4. **QueryOptimizer**: Atomic metrics, lock-free cache reads
5. **Profiler**: Atomic counters, per-thread accumulation

---

## Performance Architecture

### Memory Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                      Memory Architecture                             │
└─────────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────┐
│                     MemoryPool System                             │
├──────────────────────────────────────────────────────────────────┤
│                                                                   │
│  Thread 1                   Thread 2                   Thread N  │
│  ┌──────────────┐          ┌──────────────┐          ┌─────────┐│
│  │Thread-Local  │          │Thread-Local  │          │Thread-  ││
│  │Cache (32)    │          │Cache (32)    │          │Local    ││
│  │              │          │              │          │Cache    ││
│  │ Fast Path:   │          │ Fast Path:   │          │         ││
│  │ <100ns alloc │          │ <100ns alloc │          │         ││
│  │ >95% hit rate│          │ >95% hit rate│          │         ││
│  └──────┬───────┘          └──────┬───────┘          └────┬────┘│
│         │ Miss                    │ Miss                  │ Miss │
│         └─────────────────────────┼───────────────────────┘      │
│                                   │                              │
│                                   ▼                              │
│                      ┌─────────────────────────┐                │
│                      │    Global Memory Pool   │                │
│                      │                         │                │
│                      │  Chunk 1: [Block][Block]│                │
│                      │  Chunk 2: [Block][Block]│                │
│                      │  ...                    │                │
│                      │  Chunk N: [Block][Block]│                │
│                      │                         │                │
│                      │  Mutex-Protected        │                │
│                      │  Slow Path: ~1μs        │                │
│                      └─────────────────────────┘                │
│                                                                   │
│  Per-Bot Memory Tracking:                                        │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ BotMemoryManager                                          │   │
│  │                                                            │   │
│  │  Bot GUID → Memory Usage Map                              │   │
│  │  {                                                         │   │
│  │    Guid(123): 8.2 MB,                                     │   │
│  │    Guid(456): 7.9 MB,                                     │   │
│  │    ...                                                     │   │
│  │  }                                                         │   │
│  │                                                            │   │
│  │  Total Memory: 4.2 GB / 8.0 GB limit                      │   │
│  │  Under Pressure: false                                    │   │
│  └──────────────────────────────────────────────────────────┘   │
└──────────────────────────────────────────────────────────────────┘
```

### CPU Optimization

```
┌─────────────────────────────────────────────────────────────────────┐
│                     CPU Optimization Strategy                        │
└─────────────────────────────────────────────────────────────────────┘

Priority-Based Task Scheduling:

  ┌──────────────────────────────────────────────────────────────┐
  │ CRITICAL (0-10ms response)                                    │
  │  • Combat reactions (dodge, parry, block)                     │
  │  • Interrupt casting                                          │
  │  • Defensive cooldown triggers                                │
  │  • Emergency heals                                            │
  │                                                                │
  │  CPU Allocation: 30%                                          │
  └──────────────────────────────────────────────────────────────┘

  ┌──────────────────────────────────────────────────────────────┐
  │ HIGH (10-50ms response)                                       │
  │  • Movement updates                                           │
  │  • Position adjustments                                       │
  │  • Follow leader                                              │
  │  • Pathfinding recalc                                         │
  │                                                                │
  │  CPU Allocation: 25%                                          │
  └──────────────────────────────────────────────────────────────┘

  ┌──────────────────────────────────────────────────────────────┐
  │ NORMAL (50-200ms response)                                    │
  │  • AI decision making                                         │
  │  • Rotation execution                                         │
  │  • Target selection                                           │
  │  • Resource management                                        │
  │                                                                │
  │  CPU Allocation: 30%                                          │
  └──────────────────────────────────────────────────────────────┘

  ┌──────────────────────────────────────────────────────────────┐
  │ LOW (200-1000ms response)                                     │
  │  • Social interactions                                        │
  │  • Chat responses                                             │
  │  • Emotes                                                     │
  │                                                                │
  │  CPU Allocation: 10%                                          │
  └──────────────────────────────────────────────────────────────┘

  ┌──────────────────────────────────────────────────────────────┐
  │ IDLE (>1000ms response)                                       │
  │  • Background tasks                                           │
  │  • Database cleanup                                           │
  │  • Cache maintenance                                          │
  │                                                                │
  │  CPU Allocation: 5%                                           │
  └──────────────────────────────────────────────────────────────┘
```

---

## Integration Points

### TrinityCore Integration Points

```
┌─────────────────────────────────────────────────────────────────────┐
│               TrinityCore Integration Points (Minimal)               │
└─────────────────────────────────────────────────────────────────────┘

1. World::Update() Hook
   ┌──────────────────────────────────────────────────────────────┐
   │ Location: src/server/game/World/World.cpp                     │
   │                                                                │
   │ void World::Update(uint32 diff) {                             │
   │     // ... existing code ...                                  │
   │                                                                │
   │     #ifdef BUILD_PLAYERBOT                                    │
   │     if (sPlayerbotConfig->IsEnabled())                        │
   │         sBotScheduler->Update(diff);                          │
   │     #endif                                                     │
   │ }                                                              │
   └──────────────────────────────────────────────────────────────┘

2. Player Login Hook
   ┌──────────────────────────────────────────────────────────────┐
   │ Location: src/server/game/Entities/Player/Player.cpp          │
   │                                                                │
   │ void Player::LoadFromDB(...) {                                │
   │     // ... existing code ...                                  │
   │                                                                │
   │     #ifdef BUILD_PLAYERBOT                                    │
   │     if (IsBot())                                              │
   │         sBotSessionMgr->OnPlayerLoad(this);                   │
   │     #endif                                                     │
   │ }                                                              │
   └──────────────────────────────────────────────────────────────┘

3. Combat Event Observers
   ┌──────────────────────────────────────────────────────────────┐
   │ Location: src/server/game/Combat/ThreatManager.cpp            │
   │                                                                │
   │ void Unit::AttackedBy(Unit* attacker) {                       │
   │     // ... existing code ...                                  │
   │                                                                │
   │     #ifdef BUILD_PLAYERBOT                                    │
   │     if (BotAI* ai = GetBotAI())                               │
   │         ai->OnAttacked(attacker);                             │
   │     #endif                                                     │
   │ }                                                              │
   └──────────────────────────────────────────────────────────────┘

4. Database Schema Extensions
   ┌──────────────────────────────────────────────────────────────┐
   │ Location: sql/playerbot/00_playerbot_schema.sql               │
   │                                                                │
   │ CREATE TABLE IF NOT EXISTS `playerbot_bots` (                 │
   │     `guid` INT UNSIGNED PRIMARY KEY,                          │
   │     `account` INT UNSIGNED,                                   │
   │     `name` VARCHAR(12),                                       │
   │     `ai_config` TEXT,                                         │
   │     ...                                                        │
   │ );                                                             │
   └──────────────────────────────────────────────────────────────┘

5. Configuration System
   ┌──────────────────────────────────────────────────────────────┐
   │ Location: src/modules/Playerbot/Config/PlayerbotConfig.cpp    │
   │                                                                │
   │ bool PlayerbotConfig::LoadConfig() {                          │
   │     ConfigMgr* cfg = ConfigMgr::instance();                   │
   │     _enabled = cfg->GetBoolDefault("Playerbot.Enable", false);│
   │     ...                                                        │
   │ }                                                              │
   └──────────────────────────────────────────────────────────────┘
```

### Hook Pattern Usage

**All core integration uses hook pattern**:
- **Module code** contains all logic
- **Core code** calls hooks if enabled
- **Zero overhead** when disabled (`#ifdef BUILD_PLAYERBOT`)
- **Backward compatible** - existing functionality unchanged

---

## Summary

### Architecture Strengths

1. **Modularity**: 100% module-isolated code
2. **Performance**: Lock-free data structures, thread-local caching
3. **Scalability**: Linear scaling from 1 to 5000 bots
4. **Safety**: Thread-safe by design, minimal shared mutable state
5. **Maintainability**: Clear separation of concerns, well-defined interfaces
6. **Flexibility**: Plugin architecture, easy to extend

### Key Design Decisions

1. **No Network Sockets**: BotSession simulates packet handling internally
2. **ThreadPool**: Work-stealing queue for optimal CPU utilization
3. **MemoryPool**: Thread-local caches eliminate allocation contention
4. **Priority Scheduling**: Critical tasks (combat) get CPU priority
5. **Minimal Core Integration**: Hook/observer pattern for all TrinityCore interaction

### Performance Characteristics

- **CPU**: <0.1% per bot, <10% for 100 bots
- **Memory**: <10MB per bot, <1GB for 100 bots
- **Latency**: <1ms AI update, <50ms database query
- **Throughput**: >10,000 bot updates/second
- **Scalability**: Linear scaling to 5000+ bots

---

**End of Architecture Documentation**

*Last Updated: 2025-10-04*
*Version: 1.0*
*TrinityCore PlayerBot - Enterprise Edition*
