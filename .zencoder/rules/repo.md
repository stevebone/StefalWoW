---
description: Repository Information Overview
alwaysApply: true
---

# TrinityCore Repository Information

## Summary
TrinityCore is an open-source MMORPG server framework written in C++20, forked from MaNGOS. It provides a complete game server environment consisting of authentication server (bnetserver), world server (worldserver), and database layer. The repository includes a Playerbot module for AI-controlled player bots.

**Development Note**: While TrinityCore (master) is developed by the TrinityCore team, the Playerbot module branch (playerbot-dev) is being developed independently.

## Structure
- **src/**: Core source code
  - **server/**: Main server components (bnetserver, worldserver)
  - **common/**: Shared libraries and utilities
  - **modules/**: Optional modules including Playerbot
- **sql/**: Database scripts and migrations
- **dep/**: Third-party dependencies
- **cmake/**: Build system configuration
- **contrib/**: Utilities and helper scripts
- **doc/**: Documentation files
- **tests/**: Unit and integration tests

## Language & Runtime
**Language**: C++20  
**Build System**: CMake 3.24+  
**Package Manager**: None (dependencies included in repo)  
**Compiler Support**: MSVC 2022 (Windows), GCC 11+ (Linux), Clang 14+ (macOS)

## Dependencies
**Main Dependencies**:
- Boost 1.74+
- MySQL 9.4
- OpenSSL
- ZLib
- Protobuf
- Intel TBB (for Playerbot module)
- Parallel Hashmap (for Playerbot module)

**Development Dependencies**:
- Catch2 (testing framework)

## Build & Installation
```bash
# Basic build (without Playerbot)
cmake -B build -DCMAKE_INSTALL_PREFIX=/path/to/install
cmake --build build --config Release
cmake --install build

# With Playerbot module
cmake -B build -DBUILD_PLAYERBOT=1 -DCMAKE_INSTALL_PREFIX=/path/to/install
cmake --build build --config Release
cmake --install build
```

## Testing
**Framework**: Catch2  
**Test Location**: tests/ directory  
**Run Command**:
```bash
cd build
ctest -C Release
```

## Projects

### Core Server
**Configuration Files**: worldserver.conf, bnetserver.conf

#### Language & Runtime
**Language**: C++20  
**Build System**: CMake 3.24+  
**Database**: MySQL 9.4

#### Components
- **bnetserver**: Battle.net authentication server
- **worldserver**: Main game world server
- **Database layer**: auth, characters, and world databases

#### Build & Installation
```bash
cmake -B build
cmake --build build --config Release
```

### Playerbot Module
**Configuration File**: playerbots.conf.dist
**Branch**: playerbot-dev (independently developed)
**Location**: src/modules/Playerbot/

#### Language & Runtime
**Language**: C++20  
**Build System**: CMake (optional module)  
**Compilation Flag**: BUILD_PLAYERBOT=1

#### Dependencies
**Main Dependencies**:
- Intel TBB (Threading Building Blocks)
- Parallel Hashmap
- Boost libraries

#### Architecture
The Playerbot module follows a modular architecture with several key components:

1. **Core Module Structure**
   - PlayerbotModule: Main entry point
   - PlayerbotConfig: Configuration management
   - DependencyValidator: Validates required dependencies

2. **Session Management**
   - BotSession: Specialized WorldSession implementation
   - BotSessionMgr: High-performance session management
   - Session optimization with hibernation for inactive bots

3. **AI Framework**
   - BotAI: Core AI decision-making
   - Strategy Pattern: Combat, quest, social strategies
   - Action System: Movement, combat, spell actions
   - Trigger System: Health, combat, timer triggers
   - Value System: Data-driven decision making

4. **Class-Specific AI**
   - ClassAI: Base class for class implementations
   - Specialization System: Class specialization AI
   - Resource Management: Class-specific resources
   - Cooldown Management: Ability cooldowns

5. **Account & Character Management**
   - BotAccountMgr: Account management
   - BotNameMgr: Name generation and allocation
   - BotCharacterDistribution: Race/class distribution
   - BotCustomizationGenerator: Appearance options

6. **Lifecycle Management**
   - BotLifecycleMgr: Overall bot lifecycle
   - BotSpawner: Character creation and spawning
   - BotScheduler: Login/logout scheduling

7. **Database System**
   - PlayerbotDatabase: Database connections
   - PlayerbotDatabaseStatements: Prepared statements
   - PlayerbotMigrationMgr: Schema migrations

#### Performance Optimization
- Multi-threading with Intel TBB
- Memory optimization with hibernation
- Batch processing of bot updates
- Lock-free concurrent data structures

#### Build & Installation
```bash
# Enable Playerbot module
cmake -B build -DBUILD_PLAYERBOT=1
cmake --build build --config Release
```

#### Database
**Schema**: playerbot_accounts, playerbot_characters, playerbots_names  
**Migration Files**: sql/playerbot/01_playerbot_structure.sql

#### Development Status
The module is in active development with a phased approach:
1. **Foundation Infrastructure**: Build system, configuration, database, session management
2. **Bot Management**: Account & character creation, lifecycle management
3. **AI Core System**: AI framework, class-specific AI (current phase)
4. **Game Integration**: Group/raid, dungeon/battleground, quest automation
5. **Optimization**: Performance tuning and memory optimization
6. **Production Ready**: Admin commands, monitoring, documentation

The code structure suggests significant progress on the AI Core System with implementations for all playable classes and specializations.