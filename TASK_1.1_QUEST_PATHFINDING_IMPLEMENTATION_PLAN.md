# Task 1.1: Quest Pathfinding System - Detailed Implementation Plan
**Priority**: CRITICAL (Priority 1)
**Estimated Time**: 3 days (24 hours)
**Status**: IN PROGRESS
**Date Started**: 2025-10-13

---

## 📋 EXECUTIVE SUMMARY

This task implements a complete quest pathfinding system for the TrinityCore PlayerBot module, enabling bots to intelligently navigate to quest hubs, quest givers, and quest objectives. The implementation follows **enterprise-grade standards** with no shortcuts, comprehensive error handling, and full TrinityCore API integration.

### Current State
- **File**: `AI/Strategy/QuestStrategy.cpp:970`
- **Status**: Stubbed with TODO comment
- **Issue**: Bots cannot navigate to quest areas autonomously

### Target State
- ✅ Complete Quest Hub Database with level-appropriate zones
- ✅ TrinityCore pathfinding integration (navmesh)
- ✅ Intelligent quest giver detection and prioritization
- ✅ Performance: <1ms per pathfinding calculation
- ✅ Comprehensive testing across all starting zones

---

## 🏗️ ARCHITECTURE OVERVIEW

### Component Structure

```
src/modules/Playerbot/Quest/
├── QuestHubDatabase.h          [NEW] Quest hub data management
├── QuestHubDatabase.cpp        [NEW] Database queries and caching
├── QuestPathfinder.h           [NEW] Pathfinding logic
├── QuestPathfinder.cpp         [NEW] Route calculation
└── QuestGiverScanner.h         [NEW] NPC detection and filtering

src/modules/Playerbot/AI/Strategy/
└── QuestStrategy.cpp           [MODIFY] Integration point
```

### Data Flow

```
Player Level/Zone
    ↓
QuestHubDatabase::GetQuestHubsForLevel()
    ↓
QuestPathfinder::CalculateRoute()
    ↓
TrinityCore Pathfinding API
    ↓
MotionMaster::MoveSplinePath()
    ↓
Bot Movement
```

---

## 📦 STEP 1: QUEST HUB DATABASE (6 hours)

### 1.1 Data Structure Design (1 hour)

**File**: `src/modules/Playerbot/Quest/QuestHubDatabase.h`

```cpp
#ifndef _PLAYERBOT_QUEST_HUB_DATABASE_H
#define _PLAYERBOT_QUEST_HUB_DATABASE_H

#include "Define.h"
#include "Position.h"
#include <vector>
#include <unordered_map>
#include <mutex>

namespace PlayerBot {

/**
 * @brief Represents a quest hub location with metadata
 *
 * A quest hub is a location with multiple quest givers that provide
 * quests appropriate for a specific level range. Hubs are organized
 * by faction, zone, and level to enable efficient bot navigation.
 */
struct QuestHub
{
    uint32 hubId;                    ///< Unique identifier
    uint32 zoneId;                   ///< Zone ID (from AreaTable.dbc)
    Position location;               ///< Hub center coordinates
    uint32 minLevel;                 ///< Minimum recommended level
    uint32 maxLevel;                 ///< Maximum recommended level
    uint32 factionMask;              ///< Alliance=1, Horde=2, Both=3
    std::string name;                ///< Hub name for logging
    std::vector<uint32> questIds;    ///< Available quest IDs
    std::vector<uint32> creatureIds; ///< Quest giver creature entries
    float radius;                    ///< Search radius around hub center

    /**
     * @brief Check if this hub is appropriate for a player
     */
    bool IsAppropriateFor(Player const* player) const;

    /**
     * @brief Get distance from player to hub center
     */
    float GetDistanceFrom(Player const* player) const;
};

/**
 * @brief Manages quest hub data with caching and efficient lookup
 *
 * This class provides fast access to quest hub information using
 * multiple indices (by level, zone, faction) with thread-safe caching.
 * Data is loaded once from the world database at server startup.
 */
class QuestHubDatabase
{
public:
    /**
     * @brief Initialize the quest hub database
     * @return true if initialization successful
     *
     * Loads quest hub data from world database and builds indices.
     * Should be called once during server initialization.
     */
    static bool Initialize();

    /**
     * @brief Get quest hubs appropriate for player's level and faction
     * @param player The player to get hubs for
     * @param maxResults Maximum number of hubs to return (default: 10)
     * @return Vector of quest hubs sorted by distance
     */
    static std::vector<QuestHub const*> GetQuestHubsForPlayer(
        Player const* player,
        uint32 maxResults = 10
    );

    /**
     * @brief Get nearest quest hub to player's current location
     * @param player The player to find hub for
     * @return Nearest quest hub, or nullptr if none available
     */
    static QuestHub const* GetNearestQuestHub(Player const* player);

    /**
     * @brief Get quest hubs in a specific zone
     * @param zoneId Zone ID to search
     * @param level Player level (for filtering)
     * @param factionMask Faction mask (1=Alliance, 2=Horde, 3=Both)
     * @return Vector of quest hubs in the zone
     */
    static std::vector<QuestHub const*> GetQuestHubsInZone(
        uint32 zoneId,
        uint32 level,
        uint32 factionMask
    );

    /**
     * @brief Get all quest givers at a hub location
     * @param hub The quest hub to scan
     * @return Vector of creature entries that give quests
     */
    static std::vector<uint32> GetQuestGiversAtHub(QuestHub const* hub);

    /**
     * @brief Check if a position is within a quest hub radius
     */
    static QuestHub const* GetHubAtPosition(Position const& pos);

private:
    // Singleton pattern
    static QuestHubDatabase& Instance();
    QuestHubDatabase() = default;

    // Data loading
    bool LoadFromDatabase();
    bool LoadFromStaticData();  // Fallback if DB query fails

    // Internal storage
    std::vector<QuestHub> _hubs;
    std::unordered_map<uint32, std::vector<QuestHub*>> _hubsByZone;
    std::unordered_map<uint32, std::vector<QuestHub*>> _hubsByLevel;

    // Thread safety
    mutable std::shared_mutex _mutex;
    bool _initialized = false;
};

} // namespace PlayerBot

#endif // _PLAYERBOT_QUEST_HUB_DATABASE_H
```

### 1.2 Database Query Implementation (3 hours)

**File**: `src/modules/Playerbot/Quest/QuestHubDatabase.cpp`

**Key Implementation Points**:
1. Query world database for quest givers grouped by location
2. Calculate hub centers using spatial clustering
3. Build efficient indices for fast lookup
4. Implement thread-safe caching

**Database Query Strategy**:
```sql
-- Find quest giver clusters by location
SELECT
    c.entry,
    c.map,
    c.position_x,
    c.position_y,
    c.position_z,
    qt.MinLevel,
    qt.QuestLevel,
    qt.AllowableRaces,
    a.ID as zone_id,
    a.AreaName
FROM creature c
INNER JOIN creature_queststarter cqs ON c.entry = cqs.creature_entry
INNER JOIN quest_template qt ON cqs.quest = qt.ID
INNER JOIN AreaTable a ON c.zoneId = a.ID
WHERE c.map IN (0, 1, 530, 571)  -- Classic, Kalimdor, Outland, Northrend
GROUP BY c.entry
HAVING COUNT(DISTINCT cqs.quest) >= 3  -- At least 3 quests
ORDER BY a.ID, qt.MinLevel;
```

### 1.3 Static Data Fallback (1 hour)

**Fallback Data**: Pre-defined quest hubs for each starting zone

```cpp
// Starting zones for all races
static const QuestHub STATIC_QUEST_HUBS[] = {
    // Alliance
    {1, 9, {-8914.5f, -133.3f, 80.5f}, 1, 10, 1, "Northshire Abbey", {}, {}, 50.0f},
    {2, 14, {-6228.0f, 331.0f, 383.0f}, 1, 10, 1, "Coldridge Valley", {}, {}, 50.0f},
    {3, 1, {10329.0f, 831.0f, 1326.0f}, 1, 10, 1, "Shadowglen", {}, {}, 50.0f},

    // Horde
    {4, 12, {-618.0f, -4251.0f, 38.7f}, 1, 10, 2, "Valley of Trials", {}, {}, 50.0f},
    {5, 85, {-2917.0f, -257.0f, 53.7f}, 1, 10, 2, "Tirisfal Glades", {}, {}, 50.0f},
    {6, 141, {10311.0f, 831.0f, 1326.0f}, 1, 10, 2, "Eversong Woods", {}, {}, 50.0f},

    // And 50+ more hubs for levels 1-80...
};
```

### 1.4 Caching & Indexing (1 hour)

**Performance Requirements**:
- Index by level: O(1) lookup
- Index by zone: O(1) lookup
- Distance calculation: O(n) where n = hubs in level range (typically <20)
- Thread-safe with shared_mutex (read-heavy workload)

---

## 🗺️ STEP 2: PATHFINDING INTEGRATION (8 hours)

### 2.1 Pathfinder Class Design (2 hours)

**File**: `src/modules/Playerbot/Quest/QuestPathfinder.h`

```cpp
#ifndef _PLAYERBOT_QUEST_PATHFINDER_H
#define _PLAYERBOT_QUEST_PATHFINDER_H

#include "PathGenerator.h"
#include "Position.h"
#include "QuestHubDatabase.h"
#include <memory>

namespace PlayerBot {

/**
 * @brief Results of a pathfinding operation
 */
struct PathfindingResult
{
    bool success;                       ///< Path found successfully
    std::unique_ptr<PathGenerator> path; ///< Generated path (ownership transferred)
    float distance;                     ///< Total path distance
    uint32 calculationTimeUs;          ///< Time spent calculating (microseconds)
    std::string failureReason;         ///< Reason for failure (if any)

    bool IsValid() const { return success && path && path->GetPathType() == PATHFIND_NORMAL; }
};

/**
 * @brief Handles pathfinding for quest-related navigation
 *
 * Integrates with TrinityCore's PathGenerator to create efficient
 * routes to quest hubs, quest givers, and quest objectives.
 */
class QuestPathfinder
{
public:
    explicit QuestPathfinder(Player* bot);
    ~QuestPathfinder() = default;

    /**
     * @brief Calculate path to nearest quest hub
     * @return Pathfinding result with path data
     */
    PathfindingResult CalculatePathToNearestHub();

    /**
     * @brief Calculate path to a specific quest hub
     * @param hub Target quest hub
     * @return Pathfinding result
     */
    PathfindingResult CalculatePathToHub(QuestHub const* hub);

    /**
     * @brief Calculate path to a quest objective
     * @param questId Quest ID
     * @param objectiveIndex Which objective (0-based)
     * @return Pathfinding result
     */
    PathfindingResult CalculatePathToObjective(uint32 questId, uint32 objectiveIndex);

    /**
     * @brief Check if path to location is navigable
     * @param destination Target position
     * @return true if path exists and is walkable
     */
    bool IsNavigable(Position const& destination);

    /**
     * @brief Get estimated travel time to destination
     * @param destination Target position
     * @return Time in seconds, or 0 if not reachable
     */
    float GetEstimatedTravelTime(Position const& destination);

private:
    Player* _bot;

    // Helper methods
    PathfindingResult GeneratePath(Position const& destination);
    bool ValidatePath(PathGenerator const* path);
    float CalculatePathLength(PathGenerator const* path);
};

} // namespace PlayerBot

#endif // _PLAYERBOT_QUEST_PATHFINDER_H
```

### 2.2 TrinityCore Pathfinding API Integration (4 hours)

**Key Integration Points**:

```cpp
PathfindingResult QuestPathfinder::GeneratePath(Position const& destination)
{
    PathfindingResult result;
    auto startTime = std::chrono::high_resolution_clock::now();

    // Create path generator using TrinityCore API
    result.path = std::make_unique<PathGenerator>(_bot);

    // Calculate path using navmesh
    bool pathCalculated = result.path->CalculatePath(
        destination.GetPositionX(),
        destination.GetPositionY(),
        destination.GetPositionZ(),
        false  // forceDest = false (use nearest valid point)
    );

    // Calculate timing
    auto endTime = std::chrono::high_resolution_clock::now();
    result.calculationTimeUs = std::chrono::duration_cast<std::chrono::microseconds>(
        endTime - startTime
    ).count();

    // Validate result
    if (!pathCalculated)
    {
        result.success = false;
        result.failureReason = "PathGenerator::CalculatePath() failed";
        return result;
    }

    // Check path type
    PathType pathType = result.path->GetPathType();
    if (pathType == PATHFIND_NOPATH)
    {
        result.success = false;
        result.failureReason = "No path available (PATHFIND_NOPATH)";
        return result;
    }

    if (pathType == PATHFIND_INCOMPLETE)
    {
        TC_LOG_WARN("bot.playerbot",
            "Quest pathfinding incomplete for bot {} to ({}, {}, {})",
            _bot->GetName(),
            destination.GetPositionX(),
            destination.GetPositionY(),
            destination.GetPositionZ()
        );
        // Allow incomplete paths (bot will get as close as possible)
    }

    // Calculate distance
    result.distance = CalculatePathLength(result.path.get());
    result.success = true;

    return result;
}
```

### 2.3 Performance Optimization (1 hour)

**Optimizations**:
1. **Path Caching**: Cache recently calculated paths (LRU, max 100 entries)
2. **Early Exit**: Check straight-line distance first
3. **Incremental Updates**: Recalculate only when player moves >10 yards
4. **Async Calculation**: Offload to worker thread for paths >100 yards

### 2.4 Error Handling & Fallbacks (1 hour)

```cpp
// Fallback strategies when pathfinding fails
enum class PathfindingFallback
{
    NEAREST_VALID_POINT,  // Move to nearest navmesh point
    STRAIGHT_LINE,        // Attempt direct movement
    ALTERNATE_HUB,        // Try different quest hub
    WAIT_AND_RETRY        // Wait 5 seconds and retry
};
```

---

## 🔍 STEP 3: QUEST GIVER DETECTION (4 hours)

### 3.1 Scanner Implementation (2 hours)

**File**: `src/modules/Playerbot/Quest/QuestGiverScanner.h`

```cpp
class QuestGiverScanner
{
public:
    /**
     * @brief Scan for quest givers near bot
     * @param searchRadius Radius to search (default: 50 yards)
     * @return Vector of quest giver creatures, sorted by priority
     */
    std::vector<Creature*> ScanNearbyQuestGivers(float searchRadius = 50.0f);

    /**
     * @brief Find the best quest giver for current bot state
     * @return Creature to interact with, or nullptr
     */
    Creature* FindBestQuestGiver();

private:
    Player* _bot;

    // Priority calculation
    uint32 CalculateQuestGiverPriority(Creature* creature);
    bool HasCompletedQuestsFor(Creature* creature);
    bool HasAvailableQuestsFrom(Creature* creature);
};
```

### 3.2 Priority System (1 hour)

**Priority Rules** (highest to lowest):
1. Quest turn-in (completed quests) - Priority 100
2. Quest pickup (available, not started) - Priority 80
3. Quest progression (incomplete objectives) - Priority 60
4. Future quests (too low level) - Priority 20

### 3.3 Filtering & Validation (1 hour)

**Filters**:
- Level requirements (quest min/max level)
- Faction requirements (Alliance/Horde/Both)
- Prerequisite quests completed
- Reputation requirements met
- Class/race restrictions

---

## 🧪 STEP 4: INTEGRATION & TESTING (6 hours)

### 4.1 QuestStrategy Integration (2 hours)

**File**: `src/modules/Playerbot/AI/Strategy/QuestStrategy.cpp`

```cpp
void QuestStrategy::FindQuestHub()
{
    // Remove TODO comment at line 970
    // Implement full logic:

    // 1. Get nearest quest hub
    auto hub = QuestHubDatabase::GetNearestQuestHub(_bot);
    if (!hub)
    {
        TC_LOG_DEBUG("bot.playerbot",
            "No quest hub found for bot {} (level {})",
            _bot->GetName(), _bot->GetLevel());
        return;
    }

    // 2. Calculate path
    QuestPathfinder pathfinder(_bot);
    auto result = pathfinder.CalculatePathToHub(hub);

    if (!result.IsValid())
    {
        TC_LOG_WARN("bot.playerbot",
            "Failed to calculate path to hub {} for bot {}: {}",
            hub->name, _bot->GetName(), result.failureReason);
        return;
    }

    // 3. Start movement
    _bot->GetMotionMaster()->Clear();
    _bot->GetMotionMaster()->MoveSplinePath(result.path->GetPath());

    TC_LOG_INFO("bot.playerbot",
        "Bot {} navigating to quest hub {} ({:.1f} yards, {:.1f}ms calculation)",
        _bot->GetName(), hub->name, result.distance,
        result.calculationTimeUs / 1000.0f);
}
```

### 4.2 Unit Tests (2 hours)

**File**: `src/modules/Playerbot/Tests/QuestPathfindingTest.cpp`

```cpp
TEST(QuestPathfinding, QuestHubDatabase_Initialization)
{
    ASSERT_TRUE(QuestHubDatabase::Initialize());

    // Test data loaded
    auto hubs = QuestHubDatabase::GetQuestHubsInZone(1, 5, 1);
    ASSERT_GT(hubs.size(), 0);
}

TEST(QuestPathfinding, FindNearestHub_Alliance)
{
    // Create level 5 human player
    auto player = CreateTestPlayer(RACE_HUMAN, 5);
    player->TeleportTo(0, -8914.5f, -133.3f, 80.5f, 0.0f);

    auto hub = QuestHubDatabase::GetNearestQuestHub(player);
    ASSERT_NE(hub, nullptr);
    ASSERT_STREQ(hub->name.c_str(), "Northshire Abbey");
}

TEST(QuestPathfinding, CalculatePath_Success)
{
    auto player = CreateTestPlayer(RACE_HUMAN, 5);
    player->TeleportTo(0, -8914.5f, -133.3f, 80.5f, 0.0f);

    QuestPathfinder pathfinder(player);
    auto result = pathfinder.CalculatePathToNearestHub();

    ASSERT_TRUE(result.success);
    ASSERT_TRUE(result.IsValid());
    ASSERT_GT(result.distance, 0.0f);
    ASSERT_LT(result.calculationTimeUs, 1000);  // <1ms
}

TEST(QuestPathfinding, Performance_100Calculations)
{
    auto player = CreateTestPlayer(RACE_HUMAN, 5);
    QuestPathfinder pathfinder(player);

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 100; ++i)
    {
        pathfinder.CalculatePathToNearestHub();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start
    ).count();

    ASSERT_LT(duration, 100);  // <100ms for 100 calculations = <1ms average
}
```

### 4.3 Integration Testing (2 hours)

**Test Scenarios**:
1. Bot spawns → finds quest hub → navigates successfully
2. Bot at hub → scans quest givers → prioritizes correctly
3. Bot picks quest → calculates objective path → moves to objective
4. Edge cases: unreachable hubs, no navmesh, level boundaries

---

## 📊 PERFORMANCE TARGETS

| Metric | Target | How to Measure |
|--------|--------|----------------|
| Path calculation | <1ms | High-resolution timer |
| Database query | <10ms | Query profiling |
| Hub lookup | <0.1ms | Cached index access |
| Memory per bot | <1KB | sizeof(PathfindingResult) |
| CPU overhead | <0.01% | Profiler sampling |

---

## 🔒 ERROR HANDLING STRATEGY

### Critical Errors (Abort Operation)
- Null player pointer
- Invalid map ID
- Corrupted pathfinding data

### Recoverable Errors (Fallback)
- Hub unreachable → Try next nearest hub
- Path incomplete → Accept partial path
- Quest giver not found → Rescan after 5 seconds

### Warnings (Log Only)
- Path calculation >1ms
- No hubs in level range
- All hubs previously visited

---

## 📝 DOCUMENTATION REQUIREMENTS

### Code Documentation
- Doxygen comments for all public APIs
- Usage examples in header files
- Implementation notes in complex algorithms

### User Documentation
- Add to PLAYERBOT_USER_GUIDE.md
- Configuration options in playerbots.conf
- Troubleshooting guide for common issues

---

## ✅ ACCEPTANCE CRITERIA

Before marking this task complete, verify:

- [ ] All 4 new files created and compiling
- [ ] QuestStrategy.cpp TODO removed and implemented
- [ ] All unit tests passing
- [ ] Integration test successful with 10 different bot levels
- [ ] Performance targets met (<1ms path calculation)
- [ ] No memory leaks (valgrind/ASAN clean)
- [ ] Code reviewed against CLAUDE.md guidelines
- [ ] Documentation complete
- [ ] CMakeLists.txt updated with new files

---

## 🚀 IMPLEMENTATION ORDER

**Day 1** (8 hours):
1. Morning: Create QuestHubDatabase.h/.cpp (4 hours)
2. Afternoon: Implement database queries and caching (4 hours)

**Day 2** (8 hours):
1. Morning: Create QuestPathfinder.h/.cpp with TrinityCore integration (4 hours)
2. Afternoon: Implement QuestGiverScanner.h (4 hours)

**Day 3** (8 hours):
1. Morning: Integrate with QuestStrategy.cpp (2 hours)
2. Mid-day: Write and run unit tests (3 hours)
3. Afternoon: Integration testing and bug fixes (3 hours)

---

## 🎯 RISKS & MITIGATION

| Risk | Probability | Impact | Mitigation |
|------|------------|--------|------------|
| TrinityCore API changes | Low | High | Use stable APIs, add version checks |
| Navmesh data missing | Medium | High | Fallback to flight paths/walking |
| Performance degradation | Low | Medium | Profiling at each step, caching |
| Database query slow | Medium | Medium | Indexed queries, caching, async |

---

**Status**: READY FOR IMPLEMENTATION
**Next Step**: Begin Step 1.1 - Create QuestHubDatabase.h
**Estimated Completion**: 3 days from start
