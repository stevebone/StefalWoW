# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

TrinityCore WoW server emulator (C++20, client 12.0.1+) with a large-scale Playerbot AI module (~636K lines, ~1,629 files) that adds AI-controlled player bots for a single-player MMORPG experience.

**Branches**: `playerbot-dev` (main development), `master` (base TrinityCore / PR target)

## Build Commands

### Configure (Windows, Visual Studio 17 2022)
```bat
configure_relwithdebinfo.bat   # Primary config (optimized + debug symbols)
configure_debug.bat            # Debug config
configure_release.bat          # Release config
```
CMake flag `-DBUILD_PLAYERBOT=1` enables the Playerbot module.

### Build
```bat
cmake --build build --config RelWithDebInfo --target worldserver
cmake --build build --config Debug --target worldserver
```
Build timeout: 30 minutes. The Playerbot module splits into multiple static libraries to avoid MSVC's 4GB COFF limit.

### Syntax Check (Linux/CI)
```bash
./check-playerbot-syntax.sh
```

## Critical Rules

- **Module-first**: ALL new code goes in `src/modules/Playerbot/`. Core changes (`src/server/`) need justification and must use hook/event patterns. Never refactor core wholesale.
- **TrinityCore master is golden**: Adapt our code to TrinityCore, never the reverse. Always use TrinityCore APIs; never bypass existing systems.
- **No stubs/TODOs**: Full, complete implementations always. No simplified placeholders.
- **Check data before coding**: Always check DB2/DBC/SQL data to avoid reimplementing what already exists.
- **Commit messages**: NEVER mention Claude/AI/Co-Authored-By. NEVER mention IDA/reverse-engineering/disassembly.
- **Target client**: WoW 12.0.1+ ONLY.
- **Performance target**: 100-500 concurrent bots, <10% server impact, <0.1% CPU per bot.

## Code Style

- **Formatting**: 4 spaces, no tabs, max 160 chars/line, latin1 charset for C/C++ files (see `.editorconfig`)
- **Naming**: PascalCase classes/methods, camelCase variables, `m_` prefix for members, UPPER_SNAKE for constants
- **Include order**: PCH -> own header -> project headers -> TrinityCore headers -> external libs -> stdlib
- **Threading**: `std::shared_mutex` for read-heavy, `std::mutex` for write-heavy, never hold locks across DB calls
- **Patterns**: Use TrinityCore's typed packet API, ObjectGuid system, smart pointers, RAII

## Architecture

### Module Entry Point
`PlayerbotModule` (singleton) in `src/modules/Playerbot/PlayerbotModule.h/cpp` — initializes all subsystems, registers with TrinityCore via `PlayerbotModuleAdapter`, provides `OnWorldUpdate(diff)` callback.

### Subsystem Registry
`Core/PlayerbotSubsystemRegistry` manages 30+ subsystems with priority-ordered init/update/shutdown. Subsystems registered via `Core/SubsystemAdapters.h/cpp`. Priority levels: CRITICAL (0) > HIGH (1) > NORMAL (2) > LOW (3).

Key subsystems by init order: BotAccountMgr (100) -> BotNameMgr (110) -> BotCharacterDistribution (120) -> BotWorldSessionMgr (130) -> BotPacketRelay (140) -> BotChatCommandHandler (150).

### AI Decision-Making
- `AI/BotAI` — main AI class with state machine (SOLO, COMBAT, DEAD, TRAVELLING, QUESTING, etc.)
- Strategies activated/deactivated dynamically for behavior switching
- `BehaviorPriorityManager` handles strategy selection
- Actions use command pattern: `ExecuteAction()` (sync) and `QueueAction()` (async, thread-safe)

### Class-Specific AI
`AI/ClassAI/ClassAI` extends BotAI with class-specific combat via `OnCombatUpdate()`. One subclass per WoW class. ClassAI ONLY handles combat; never controls movement.

### Threading Model
Worker threads produce actions via lock-free `BotActionQueue`. Main thread (`World::Update`) consumes and executes them in batches via `BotActionProcessor`. No shared mutable state between threads. `Threading/LockHierarchy.h` prevents deadlocks.

### Combat Event System
`Combat/CombatEvents.h` — event bus with typed events (spell, damage, healing, CC, threat). Priority levels: CRITICAL (interrupts, immediate) > HIGH (damage, 50ms) > MEDIUM (DoTs, 200ms) > LOW (state changes, 500ms). BotAI subscribes as `IEventHandler`.

### Movement System
`Movement/UnifiedMovementCoordinator` coordinates all movement. Sub-components: `BotMovementController` (state machine), pathfinding, road network graph, `MovementPriorityMapper` (arbitration). Migration in progress from direct `MotionMaster` calls to `BotMovementController`.

### Domain Event Buses
`Core/Events/GenericEventBus` — template-based event distribution with separate buses for: Groups, Loot, Quests, Auras, Cooldowns, Resources, Social, Auction, NPC, Instance, Profession.

### Hook System
`Core/PlayerBotHooks.h` — 8 hook points into TrinityCore (mainly Group.cpp). Observer pattern, nullable callbacks, <1μs overhead per hook.

### Database
- Module DB schema: `sql/playerbot/` (numbered migration files)
- `Database/PlayerbotDatabase` — module-specific DB
- `Database/PlayerbotMigrationMgr` — schema migrations
- `Database/PlayerbotCharacterDBInterface` — character DB access

### Bot Lifecycle
`Lifecycle/BotSpawner` (spawning/despawning), `BotFactory` (creation), `BotSaveController` (persistence). Instance bot pool: `Lifecycle/Instance/` with JIT factory and warm pool.

### Sessions
`Session/BotWorldSessionMgr` — session lifecycle. `Session/BotPacketRelay` — packet handling. `Session/AsyncBotInitializer` — async login flow.

### Dependencies
- CMake 3.24+, C++20, MySQL 9.4, Boost 1.74+
- Intel TBB (vendored fallback in `deps/tbb/`)
- MSVC 19.30+ (VS2022), GCC 11+, or Clang 14+

## File Modification Hierarchy

1. **PREFERRED** — Module-only (`src/modules/Playerbot/`), zero core modifications
2. **ACCEPTABLE** — Minimal core hooks via observer/event pattern only
3. **CAREFUL** — Core extension points with documented justification
4. **FORBIDDEN** — Core refactoring or breaking existing functionality

## MCP Tools

Research protocol: use BOTH `trinitycore` MCP (game data) AND `serena` (semantic code navigation) for any game mechanics research. Additional MCPs: `trinity-database` (MySQL), `sequential-thinking`, `github`, `context7`.
