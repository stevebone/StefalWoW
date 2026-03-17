# Bot Lifecycle Management - Implementation Roadmap

## Executive Summary
**Status**: BotSpawner ✅ Complete | Remaining Components: 70% of lifecycle system
**Current Sprint**: Phase 2A - BotScheduler Implementation
**Timeline**: 3 sprints to complete full lifecycle management system
**Priority**: High - Required for realistic bot behavior patterns

## Implementation Status

### ✅ Completed Components
- **BotSpawner**: Intelligent population management with zone balancing
- **BotCustomizationGenerator**: Visual randomization system
- **Foundation Infrastructure**: CMake integration, TBB validation

### 🎯 Remaining Critical Components

#### **1. BotScheduler (IMMEDIATE PRIORITY)**
**Files**: `src/modules/Playerbot/Lifecycle/BotScheduler.{h,cpp}`
**Purpose**: Realistic login/logout patterns and scheduling intelligence

**Core Implementation Requirements**:
```cpp
class BotScheduler {
    // Activity pattern management
    void LoadActivityPatterns();
    void RegisterPattern(std::string const& name, ActivityPattern const& pattern);
    void AssignPattern(ObjectGuid guid, std::string const& patternName);

    // Schedule management
    void ScheduleBot(ObjectGuid guid);
    void ScheduleLogin(ObjectGuid guid, std::chrono::system_clock::time_point when);
    void ScheduleLogout(ObjectGuid guid, std::chrono::system_clock::time_point when);

    // Time calculations with realistic patterns
    std::chrono::system_clock::time_point CalculateNextLogin(ObjectGuid guid);
    std::chrono::system_clock::time_point CalculateSessionEnd(ObjectGuid guid);

    // Background processing
    void ProcessSchedule(); // Main update loop
    void ExecuteScheduledAction(ScheduleEntry const& entry);
};
```

**Key Data Structures**:
```cpp
struct ActivityPattern {
    std::string name;
    std::vector<std::pair<uint32, uint32>> activeHours; // 0-23 hour ranges
    std::vector<uint32> activeDays; // 0-6 days of week
    float loginProbability;
    uint32 minSessionDuration, maxSessionDuration;
    uint32 averageSessionsPerDay;
    bool preferPeakHours;
};

struct ScheduleEntry {
    ObjectGuid botGuid;
    std::chrono::system_clock::time_point executeTime;
    enum Action { LOGIN, LOGOUT, ZONE_CHANGE, ACTIVITY_CHANGE, HEARTBEAT };
    Action action;
    uint32 data1, data2;
};
```

#### **2. BotLifecycleMgr (COORDINATION HUB)**
**Files**: `src/modules/Playerbot/Lifecycle/BotLifecycleMgr.{h,cpp}`
**Purpose**: Central coordinator integrating all lifecycle systems

**Core Implementation Requirements**:
```cpp
class BotLifecycleMgr {
    // Lifecycle coordination
    bool StartBot(ObjectGuid guid);
    bool StopBot(ObjectGuid guid);
    void MaintainPopulation(); // Main coordination loop

    // Population management
    void BalancePopulation();
    void BalanceZonePopulation(uint32 zoneId);
    void UpdateZoneTargets();

    // Event system
    void TriggerEvent(LifecycleEvent event, ObjectGuid guid, uint32 data1 = 0, uint32 data2 = 0);
    void RegisterEventHandler(LifecycleEvent event, LifecycleEventHandler handler);

    // Integration with other systems
    void InitializeWithSpawner();
    void InitializeWithScheduler();
    void InitializeWithDatabase();

    // Statistics and monitoring
    LifecycleStats GetStatistics() const;
    void LogLifecycleEvent(ObjectGuid guid, LifecycleEventType type, uint32 data1, uint32 data2);
};
```

#### **3. Database Schema (FOUNDATION)**
**Files**: `sql/playerbot/003_lifecycle_management.sql`
**Purpose**: Persistent storage for all lifecycle data

**Required Tables**:
```sql
-- Bot spawn/despawn analytics and tracking
CREATE TABLE `playerbot_spawn_log` (
    `id` INT UNSIGNED AUTO_INCREMENT,
    `guid` INT UNSIGNED NOT NULL,
    `zone_id` INT UNSIGNED NOT NULL,
    `map_id` INT UNSIGNED NOT NULL,
    `spawn_time` TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    `despawn_time` TIMESTAMP NULL,
    `session_duration` INT UNSIGNED NULL,
    `spawn_reason` ENUM('scheduled', 'population_balance', 'manual', 'group_request'),
    `despawn_reason` ENUM('scheduled', 'timeout', 'manual', 'server_shutdown', 'error'),
    PRIMARY KEY (`id`),
    INDEX `idx_guid_spawn_time` (`guid`, `spawn_time`),
    INDEX `idx_zone_spawn_time` (`zone_id`, `spawn_time`)
);

-- Activity pattern definitions for scheduling
CREATE TABLE `playerbot_activity_patterns` (
    `id` INT UNSIGNED AUTO_INCREMENT,
    `name` VARCHAR(50) NOT NULL UNIQUE,
    `active_hours` JSON NOT NULL, -- Array of [start, end] hour pairs
    `active_days` JSON NOT NULL,  -- Array of weekday numbers (0-6)
    `login_probability` FLOAT NOT NULL DEFAULT 1.0,
    `min_session_duration` INT UNSIGNED NOT NULL DEFAULT 1800,
    `max_session_duration` INT UNSIGNED NOT NULL DEFAULT 14400,
    `sessions_per_day` INT UNSIGNED NOT NULL DEFAULT 2,
    `prefer_peak_hours` BOOLEAN NOT NULL DEFAULT TRUE,
    `created_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`)
);

-- Individual bot schedules and assignments
CREATE TABLE `playerbot_schedules` (
    `guid` INT UNSIGNED NOT NULL,
    `pattern_name` VARCHAR(50) NOT NULL,
    `next_login` TIMESTAMP NULL,
    `next_logout` TIMESTAMP NULL,
    `total_sessions` INT UNSIGNED DEFAULT 0,
    `total_playtime` INT UNSIGNED DEFAULT 0,
    `last_update` TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (`guid`),
    FOREIGN KEY (`pattern_name`) REFERENCES `playerbot_activity_patterns`(`name`)
);

-- Zone population management and targets
CREATE TABLE `playerbot_zone_populations` (
    `zone_id` INT UNSIGNED NOT NULL,
    `map_id` INT UNSIGNED NOT NULL,
    `min_bots` INT UNSIGNED DEFAULT 0,
    `max_bots` INT UNSIGNED DEFAULT 50,
    `target_bots` INT UNSIGNED DEFAULT 10,
    `current_bots` INT UNSIGNED DEFAULT 0,
    `min_level` TINYINT UNSIGNED DEFAULT 1,
    `max_level` TINYINT UNSIGNED DEFAULT 80,
    `bot_density` FLOAT DEFAULT 0.1,
    `last_update` TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (`zone_id`),
    INDEX `idx_map_id` (`map_id`)
);

-- Lifecycle event logging for analytics
CREATE TABLE `playerbot_lifecycle_events` (
    `id` BIGINT UNSIGNED AUTO_INCREMENT,
    `event_type` ENUM('bot_created', 'bot_spawned', 'bot_despawned', 'bot_deleted',
                      'session_start', 'session_end', 'zone_changed', 'level_changed',
                      'activity_changed', 'pattern_assigned') NOT NULL,
    `guid` INT UNSIGNED NOT NULL,
    `data1` INT UNSIGNED DEFAULT 0, -- Context-specific data
    `data2` INT UNSIGNED DEFAULT 0, -- Context-specific data
    `timestamp` TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    INDEX `idx_guid_timestamp` (`guid`, `timestamp`),
    INDEX `idx_event_type_timestamp` (`event_type`, `timestamp`)
);
```

#### **4. Configuration System Expansion**
**Files**: `src/modules/Playerbot/Config/PlayerbotConfig.{h,cpp}`
**Purpose**: Extended configuration options for lifecycle management

**New Configuration Options**:
```ini
# Lifecycle Management
Playerbot.Lifecycle.Enable = 1
Playerbot.Lifecycle.Scheduling.Enable = 1
Playerbot.Lifecycle.RealisticPatterns = 1

# Scheduling Configuration
Playerbot.Schedule.LookaheadMinutes = 60
Playerbot.Schedule.HeartbeatInterval = 300
Playerbot.Schedule.IdleTimeout = 1800
Playerbot.Schedule.PeakHourMultiplier = 2.0
Playerbot.Schedule.OffPeakMultiplier = 0.5

# Population Management
Playerbot.Population.GlobalMaxBots = 1000
Playerbot.Population.ZoneMaxBots = 50
Playerbot.Population.DefaultDensity = 0.1
Playerbot.Population.BalanceInterval = 60
```

#### **5. Testing Infrastructure**
**Files**: `src/modules/Playerbot/Tests/Lifecycle/`
**Purpose**: Comprehensive validation and performance testing

**Test Components**:
```cpp
// Core functionality tests
TEST_F(BotSchedulerTest, ActivityPatternLoading);
TEST_F(BotSchedulerTest, LoginTimeCalculation);
TEST_F(BotSchedulerTest, SessionDurationCalculation);
TEST_F(BotSchedulerTest, PeakHourWeighting);

// Integration tests
TEST_F(LifecycleIntegrationTest, SpawnerSchedulerIntegration);
TEST_F(LifecycleIntegrationTest, DatabasePersistence);
TEST_F(LifecycleIntegrationTest, PopulationBalancing);

// Performance tests
TEST_F(LifecyclePerformanceTest, ScheduleProcessing1000Bots);
TEST_F(LifecyclePerformanceTest, ConcurrentScheduleUpdates);
TEST_F(LifecyclePerformanceTest, MemoryUsageValidation);
```

## Three-Sprint Implementation Plan

### **Sprint 2A (Week 11) - Immediate Priority**
**Goal**: Core scheduling intelligence and database foundation

**Tasks**:
1. **BotScheduler Implementation** (Days 1-3)
   - Create BotScheduler.h with class structure and method signatures
   - Implement core scheduling algorithms and time calculations
   - Add Intel TBB priority queue for scheduled actions
   - Create basic activity patterns (default, peak/off-peak)

2. **Database Schema Creation** (Days 4-5)
   - Design and implement lifecycle management tables
   - Create migration scripts for database versioning
   - Add prepared statements for lifecycle operations
   - Test database integration with existing systems

3. **CMake Integration** (Day 6)
   - Add BotScheduler to build system
   - Update include paths and source groups
   - Verify compilation and basic functionality

**Deliverables**:
- Working BotScheduler with basic scheduling functionality
- Complete database schema with migration support
- Updated build system with proper integration

### **Sprint 2B (Week 12) - Coordination & Intelligence**
**Goal**: Advanced coordination and intelligent population management

**Tasks**:
1. **BotLifecycleMgr Implementation** (Days 1-3)
   - Create central coordinator class
   - Implement event-driven architecture
   - Add population balancing algorithms
   - Integrate with existing BotSpawner

2. **Advanced Scheduling Features** (Days 4-5)
   - Complex activity patterns (casual, hardcore, weekend)
   - Session persistence across server restarts
   - Dynamic pattern assignment based on bot characteristics
   - Peak hour detection and weighting

3. **Performance Optimization** (Day 6)
   - Intel TBB parallel processing integration
   - Memory optimization for large bot populations
   - Concurrent data structure implementation
   - Performance profiling and optimization

**Deliverables**:
- Complete BotLifecycleMgr coordination system
- Advanced scheduling with realistic patterns
- Performance-optimized implementation

### **Sprint 2C (Week 13) - Testing & Production Readiness**
**Goal**: Comprehensive testing and production deployment

**Tasks**:
1. **Testing Suite Implementation** (Days 1-3)
   - Unit tests for all lifecycle components
   - Integration tests with existing systems
   - Performance stress testing (500+ bots)
   - Memory leak detection and validation

2. **Admin Interface & Monitoring** (Days 4-5)
   - Admin commands for lifecycle management
   - Real-time statistics and monitoring
   - Debug commands for pattern testing
   - Performance metrics dashboard

3. **Documentation & Finalization** (Day 6)
   - Complete API documentation
   - Performance benchmarking results
   - Deployment guides and configuration examples
   - Final integration testing

**Deliverables**:
- Production-ready lifecycle management system
- Comprehensive testing and validation
- Complete documentation and admin tools

## Success Metrics

### **Performance Targets**:
- **Schedule Processing**: <1ms per bot per update cycle
- **Memory Usage**: <5KB per scheduled bot
- **Database Operations**: <10ms query response time (P95)
- **Concurrent Bots**: Linear scaling to 1000+ bots
- **Spawn Integration**: <50ms total time from schedule trigger to bot spawn

### **Functionality Targets**:
- **Realistic Patterns**: Distinguishable casual vs hardcore behavior
- **Peak Hour Simulation**: 2x population during peak hours
- **Session Persistence**: Schedule survival across server restarts
- **Population Balance**: Automatic zone distribution optimization
- **Event Integration**: Complete lifecycle event tracking

## Risk Mitigation

### **Technical Risks**:
- **TBB Integration Complexity**: Mitigate with incremental implementation
- **Database Performance**: Use async operations and connection pooling
- **Memory Usage**: Implement object pooling and smart pointers
- **Timing Precision**: Use high-resolution clocks for accurate scheduling

### **Integration Risks**:
- **Session Manager Conflicts**: Coordinate with existing session lifecycle
- **Database Schema Conflicts**: Version migration scripts carefully
- **Performance Impact**: Profile and optimize each component incrementally

## Dependencies

### **Internal Dependencies**:
- ✅ **BotSpawner**: Population management foundation (completed)
- ✅ **BotSessionMgr**: Session lifecycle management (Phase 1)
- ✅ **PlayerbotConfig**: Configuration system (Phase 1)
- ⏳ **BotDatabasePool**: Async database operations (Phase 1)

### **External Dependencies**:
- **Intel TBB**: Parallel processing and concurrent data structures
- **MySQL 9.4**: Database storage and prepared statements
- **Boost**: Smart pointers and additional utilities
- **TrinityCore APIs**: Integration with game world systems

This roadmap provides a clear path to complete the Bot Lifecycle Management system, building upon the solid foundation of the BotSpawner to create realistic, intelligent bot behavior patterns that enhance the single-player MMORPG experience.