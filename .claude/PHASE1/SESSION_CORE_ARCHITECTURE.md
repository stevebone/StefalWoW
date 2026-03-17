# Bot Session Core Architecture - Complete Implementation Specification

## Overview
High-performance, socketless session system with enterprise-grade optimizations for TrinityCore Playerbots.

## Architecture Diagram (CORRECTED)
```
TrinityCore Core (UNCHANGED)     Playerbot Module (ISOLATED)
┌─────────────────────────┐     ┌─────────────────────────────┐
│ WorldSession Base       │────▶│ BotSession (inheritance)     │
│ World Packet System     │◀────│ Packet Queue System         │
│ Player System          │◀────│ Hibernation Manager         │
│ AccountMgr (BNet+legacy)│◀────│ Bot Account Creation       │
│ CharacterDB (chars)    │◀────│ Bot Character Creation      │
└─────────────────────────┘     │                             │
                                │ BotDatabasePool (metadata)   │
                                │ ├─ AI State Storage          │
                                │ ├─ Bot Settings Cache        │
                                │ ├─ Performance Metrics       │
                                │ └─ Links to Trinity DBs      │
                                │                             │
                                │ Memory Management           │
                                │ ├─ Object Pools             │
                                │ ├─ Hibernation Cache        │
                                │ └─ Circular Buffers         │
                                │                             │
                                │ CPU Optimization            │
                                │ ├─ Intel TBB Thread Pool    │
                                │ ├─ Batch Queue Processing   │
                                │ ├─ Lock-free Structures     │
                                │ └─ SIMD Operations          │
                                └─────────────────────────────┘
```

## Core Components

### 1. BotSession (Socketless WorldSession)
**File**: `src/modules/Playerbot/Session/BotSession.h/cpp`
- **Purpose**: Virtual session without network socket
- **Dependencies**: Intel TBB, boost::circular_buffer, boost::object_pool
- **Performance**: < 500KB RAM per active session, < 5KB hibernated

### 2. BotSessionMgr (Thread Pool Manager)
**File**: `src/modules/Playerbot/Session/BotSessionMgr.h/cpp`
- **Purpose**: High-performance session pool with work-stealing
- **Dependencies**: Intel TBB, parallel_hashmap, boost::lockfree
- **Performance**: Support 100-5000 concurrent sessions

### 3. BotDatabasePool (Bot Metadata Layer)
**File**: `src/modules/Playerbot/Database/BotDatabasePool.h/cpp`
- **Purpose**: Handle ONLY bot-specific metadata (AI state, settings, metrics)
- **Integration**: Links to accounts/characters in Trinity's standard databases
- **Dependencies**: boost::asio, MySQL Connector
- **Performance**: < 10ms query time (P95), > 80% cache hit rate

## Critical Performance Requirements

### Memory Constraints
- **Active Session**: < 500KB RAM per session
- **Hibernated Session**: < 5KB RAM per session
- **Memory Reduction**: 99% reduction during hibernation
- **Total Impact**: < 10MB per bot instance

### CPU Constraints
- **Packet Processing**: < 500ns per packet
- **AI Decision Time**: < 50ms (P95)
- **CPU per Bot**: < 0.05% (measured with perf)
- **Batch Processing**: Minimum 16 items per batch

### Database Performance
- **Query Response**: < 10ms (P95)
- **Cache Hit Rate**: > 80%
- **Connection Pool**: 4 async + 2 sync threads
- **Result TTL**: 60 seconds for cached queries

## Mandatory Dependencies

### Intel Threading Building Blocks (TBB)
```cmake
find_package(TBB REQUIRED)
target_link_libraries(playerbot PRIVATE TBB::tbb)
```
**Usage**: Work-stealing thread pool, concurrent containers

### Parallel Hashmap
```cmake
find_package(phmap REQUIRED)
```
**Usage**: Better concurrency than std::unordered_map

### Boost Libraries
```cmake
find_package(Boost REQUIRED COMPONENTS system thread)
```
**Components**:
- `boost::circular_buffer` - Fixed-size packet queues
- `boost::object_pool` - Zero-allocation after warmup
- `boost::lockfree` - Lock-free data structures
- `boost::asio` - Async database operations

### MySQL Connector C++
```cmake
find_package(MySQL REQUIRED)
```
**Usage**: Isolated database connection pool

## Trinity Integration Rules

### WorldSession Override Rules
```cpp
// ONLY these 3 methods may be overridden:
void SendPacket(WorldPacket const* packet, bool forced = false) override;
void QueuePacket(WorldPacket&& packet) override;
void Update(uint32 diff) override;

// FORBIDDEN - DO NOT override any other WorldSession methods
```

### Database Usage Rules (CORRECTED)
```cpp
// CORRECT - Use Trinity's standard systems for accounts/characters
// ✅ sAccountMgr->CreateBattlenetAccount(email, password) // Creates BNet + linked legacy account
// ✅ Player::Create(...) // Creates in characters.characters
// ✅ CharacterDatabase.Execute() // For character operations via Trinity APIs
// ✅ LoginDatabase.Execute() // For account operations via Trinity APIs

// PLAYERBOT DATABASE - Only for bot-specific metadata
// ✅ sBotDBPool->ExecuteAsync() // AI state, behavior settings
// ✅ sBotDBPool->Query() // Bot configuration, performance metrics
// ✅ sBotDBPool->CacheResult() // Cache bot-specific queries

// FORBIDDEN - Direct world database access (use Trinity's world systems)
// ❌ WorldDatabase.Query() // Use existing world data access patterns
```

### Memory Management Rules
```cpp
// REQUIRED - Use object pools and circular buffers
boost::circular_buffer<WorldPacket> _packets{256};  // Fixed size
boost::object_pool<WorldPacket> _packetPool;        // Zero allocation

// FORBIDDEN - Unbounded containers
// ❌ std::vector without size limit
// ❌ std::queue without size limit
// ❌ Dynamic allocation in hot paths
```

## Build System Integration

### CMakeLists.txt Requirements
```cmake
# Find required dependencies
find_package(TBB REQUIRED)
find_package(phmap REQUIRED)
find_package(Boost REQUIRED COMPONENTS system thread)

# Session library
add_library(playerbot-session STATIC
    Session/BotSession.cpp
    Session/BotSessionMgr.cpp
    Database/BotDatabasePool.cpp)

target_link_libraries(playerbot-session
    PRIVATE
        TBB::tbb
        phmap::phmap
        Boost::system
        Boost::thread
        ${MYSQL_LIBRARIES})

target_include_directories(playerbot-session
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE ${MYSQL_INCLUDE_DIRS})
```

### Conditional Compilation
```cpp
#ifdef PLAYERBOT_ENABLED
    // Session management code
    #include "Session/BotSessionMgr.h"
    sBotSessionMgr->UpdateAllSessions(diff);
#endif
```

## Quality Assurance Requirements

### Unit Tests
- **Coverage**: > 80% line coverage
- **Performance Tests**: All benchmarks must pass
- **Memory Tests**: Valgrind clean, no leaks
- **Thread Safety**: ThreadSanitizer clean

### Benchmarks
- **Packet Processing**: < 500ns per packet
- **Session Creation**: < 1ms per session
- **Hibernation Time**: < 10ms per session
- **Database Queries**: < 10ms (P95)

### Integration Tests
- **BUILD_PLAYERBOT=OFF**: Must produce identical binary
- **Zero Core Impact**: No performance regression
- **Cross-Platform**: Windows MSVC + Linux GCC builds

## Next Steps

1. **Split into focused implementation files**:
   - `SESSION_BOTSESSION_SPEC.md` - BotSession complete specification
   - `SESSION_MANAGER_SPEC.md` - BotSessionMgr complete specification
   - `SESSION_DATABASE_SPEC.md` - BotDatabasePool complete specification
   - `SESSION_PERFORMANCE_TESTS.md` - Complete test specifications

2. **Implement dependencies first**:
   - Update CMakeLists.txt with all required libraries
   - Verify all dependencies are available
   - Create dependency validation tests

3. **Implement components in order**:
   - BotDatabasePool (foundation)
   - BotSession (core functionality)
   - BotSessionMgr (orchestration)
   - Performance tests and validation

**NO SHORTCUTS. NO SIMPLIFICATION. ENTERPRISE-GRADE IMPLEMENTATION ONLY.**