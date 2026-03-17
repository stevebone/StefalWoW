/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_DOUBLE_BUFFERED_SPATIAL_GRID_H
#define PLAYERBOT_DOUBLE_BUFFERED_SPATIAL_GRID_H

#include "Define.h"
#include "Threading/LockHierarchy.h"
#include "ObjectGuid.h"
#include "Position.h"
#include <unordered_map>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>

class Map;
class Creature;
class Player;
class GameObject;
class DynamicObject;
class AreaTrigger;

namespace Playerbot
{

/**
 * @brief Efficient cell key for sparse grid storage
 *
 * Combines X and Y coordinates into a single 64-bit key for O(1) hash lookup.
 * Memory-efficient: Only populated cells are stored (sparse representation).
 *
 * OLD APPROACH (DENSE): 512x512 array = 262,144 cells × 120 bytes = 31.46 MB per buffer
 * NEW APPROACH (SPARSE): Only ~1,000-5,000 populated cells × 120 bytes = 0.12-0.6 MB per buffer
 *
 * MEMORY SAVINGS: ~98% reduction (from ~63 MB to ~1.2 MB per map)
 */
struct CellKey
{
    uint32 x{0};
    uint32 y{0};

    CellKey() = default;
    CellKey(uint32 cellX, uint32 cellY) : x(cellX), y(cellY) {}

    bool operator==(CellKey const& other) const
    {
        return x == other.x && y == other.y;
    }

    // Pack into uint64 for efficient storage/comparison
    uint64 ToPackedKey() const
    {
        return (static_cast<uint64>(x) << 32) | static_cast<uint64>(y);
    }

    static CellKey FromPackedKey(uint64 packed)
    {
        return CellKey(
            static_cast<uint32>(packed >> 32),
            static_cast<uint32>(packed & 0xFFFFFFFF)
        );
    }
};

/**
 * @brief Hash function for CellKey (for use in unordered_map)
 */
struct CellKeyHash
{
    size_t operator()(CellKey const& key) const noexcept
    {
        // Use packed key directly as hash (perfect distribution for grid coordinates)
        return static_cast<size_t>(key.ToPackedKey());
    }
};

/**
 * @class DoubleBufferedSpatialGrid
 * @brief Lock-free SPARSE spatial grid for 5000+ concurrent bot queries with ZERO TrinityCore core modifications
 *
 * ARCHITECTURE (v2 - MEMORY OPTIMIZED):
 * - SPARSE storage: Only populated cells are allocated (saves ~98% memory)
 * - Two sparse hash maps (Buffer A and Buffer B) for double buffering
 * - Synchronous updates from Map::Update (no background thread)
 * - Atomic buffer swap after update complete
 * - Bots query active buffer with zero lock contention
 *
 * MEMORY COMPARISON:
 * - OLD (Dense):  512×512×2 buffers × 120 bytes = 62.9 MB per map (EMPTY!)
 * - NEW (Sparse): ~2,000 cells × 120 bytes × 2 = ~0.5 MB per map (typical)
 * - SAVINGS: 62.4 MB per map → ~3.1 GB saved across 50 maps!
 *
 * PERFORMANCE TARGETS:
 * - Query latency: 1-5μs per bot (unchanged)
 * - Memory: ~0.5-2 MB per map (was ~63 MB)
 * - Update overhead: 2-5ms per 100ms tick (unchanged)
 * - Scales linearly to 10,000+ bots
 *
 * THREAD SAFETY:
 * - Main thread: Updates inactive buffer during Map::Update
 * - Bot threads: Read from active buffer (atomic load, no locks)
 * - Buffer swap: Atomic pointer exchange
 *
 * MODULE INTEGRATION:
 * - Lives in src/modules/Playerbot/Spatial/ (NO core files modified)
 * - Created by BotWorldSessionMgr::Initialize()
 * - Uses TrinityCore's existing ObjectAccessor for thread-safe reads
 */
class TC_GAME_API DoubleBufferedSpatialGrid
{
public:
    // Grid constants (matches TrinityCore's spatial layout)
    static constexpr uint32 CELLS_PER_GRID = 8;
    static constexpr uint32 GRIDS_PER_MAP = 64;
    static constexpr uint32 TOTAL_CELLS = GRIDS_PER_MAP * CELLS_PER_GRID; // 512
    static constexpr float CELL_SIZE = 66.6666f; // yards
    static constexpr uint32 UPDATE_INTERVAL_MS = 100; // 10 Hz update rate

    /**
     * @brief ENTERPRISE-GRADE Immutable snapshot of creature data for thread-safe bot queries
     *
     * OPTION C: COMPLETE ENHANCEMENT - All 60+ fields from TrinityCore Creature entity
     *
     * CRITICAL: This snapshot is populated ONCE per update cycle from the main thread,
     * then read MANY times by worker threads. No pointers, no references - only POD data.
     *
     * Memory footprint: ~500 bytes per creature (5000 creatures = 2.5 MB)
     * RAM increase: +404 bytes per creature vs old schema
     *
     * DESIGN: Future-proof complete data capture - never need to modify schema again
     */
    struct CreatureSnapshot
    {
        // ===== IDENTITY (CRITICAL) =====
        ObjectGuid guid;
        uint32 entry{0};
        uint64 spawnId{0};  // Database spawn reference

        // ===== POSITION & MOVEMENT (CRITICAL) =====
        Position position;  // Current world position (x, y, z, orientation)
        float orientation{0.0f};  // Facing angle
        uint32 mapId{0};
        uint32 instanceId{0};
        uint32 zoneId{0};
        uint32 areaId{0};

        // Movement behavior
        uint8 defaultMovementType{0};  // IDLE, RANDOM, WAYPOINT
        uint32 waypointPathId{0};  // Current patrol path ID
        uint32 currentWaypointId{0};  // Current waypoint node
        Position homePosition;  // Spawn point / reset position
        float wanderDistance{0.0f};  // Patrol radius for random movement
        float moveSpeed{0.0f};  // Current movement speed
        bool isMoving{false};

        // ===== COMBAT & THREAT (CRITICAL) =====
        uint64 health{0};
        uint64 maxHealth{0};
        uint8 level{0};
        bool isInCombat{false};
        ObjectGuid victim;  // Current combat target GUID
        uint32 unitState{0};  // UNIT_STATE_* flags (stunned, rooted, casting, etc.)
        uint8 reactState{0};  // REACT_PASSIVE, REACT_DEFENSIVE, REACT_AGGRESSIVE
        uint32 attackersCount{0};  // Number of units attacking this creature
        time_t lastDamagedTime{0};  // For evade detection
        uint32 attackTimer{0};  // Melee swing timer (milliseconds until next attack)
        float combatReach{0.0f};  // Melee engagement distance
        float boundingRadius{0.0f};  // Collision/hitbox radius
        uint32 npcFlags{0};  // UNIT_NPC_FLAG_* (vendor, trainer, questgiver, etc.)
        bool isHostile{false};  // Pre-calculated hostility to players (red mobs - FactionTemplateEntry::IsHostileToPlayers)
        bool isAttackable{false};  // Can be attacked by players (hostile OR neutral, excludes friendly)
        bool isEngaged{false};  // In engaged combat state (not just threat-only)
        bool canNotReachTarget{false};  // Evade flag - target unreachable

        // ===== ATTRIBUTES (HIGH) =====
        uint8 race{0};
        uint8 classId{0};
        uint32 faction{0};
        uint8 gender{0};
        uint8 standState{0};  // UNIT_STAND_STATE_* (sitting, kneeling, dead, etc.)

        // ===== CREATURE-SPECIFIC (HIGH/CRITICAL) =====
        uint8 classification{0};  // CREATURE_ELITE_NORMAL, ELITE, RARE, RARE_ELITE, BOSS, MINUS
        bool isElite{false};
        bool isWorldBoss{false};
        bool isDungeonBoss{false};
        bool canHaveLoot{false};
        uint16 lootMode{0};  // LOOT_MODE_* bitmask
        uint8 currentEquipmentId{0};  // Equipment set ID
        uint32 corpseDelay{0};  // Corpse despawn timer (milliseconds)
        time_t respawnTime{0};  // Next respawn timestamp
        uint32 respawnDelay{0};  // Respawn interval (seconds)
        bool isRacialLeader{false};
        bool isCivilian{false};
        bool isGuard{false};
        float sparringHealthPct{0.0f};  // PvP sparring health threshold (if applicable)

        // ===== STATIC FLAGS (MEDIUM) =====
        bool isUnkillable{false};  // CREATURE_STATIC_FLAG_UNKILLABLE
        bool isSessile{false};  // Permanently rooted, cannot move
        bool canMelee{false};  // Can perform melee attacks
        bool canGiveExperience{false};  // Awards XP on kill
        bool isIgnoringFeignDeath{false};  // Pierces Feign Death
        bool isIgnoringSanctuary{false};  // Ignores sanctuary spell effects

        // ===== DISPLAY & EQUIPMENT (MEDIUM) =====
        uint32 displayId{0};  // Visual model ID
        uint32 mountDisplayId{0};  // Mount visual (if mounted)
        bool isMounted{false};
        bool canFly{false};
        bool canSwim{false};
        bool isAquatic{false};  // Water-only creature
        bool isFloating{false};  // Gravity disabled

        // ===== QUEST & LOOT (HIGH) =====
        bool isDead{false};  // For quest objectives and skinning
        bool isTappedByOther{false};  // Loot rights assigned to another player
        bool isSkinnable{false};  // Can be skinned for leather/scales
        bool hasBeenLooted{false};  // Corpse has been looted
        bool hasQuestGiver{false};  // NPC can give/complete quests
        uint32 questGiverFlags{0};  // Quest giver capabilities bitmask

        // ===== VISIBILITY & INTERACTION (MEDIUM) =====
        bool isVisible{false};  // Can be seen by bots
        float interactionRange{0.0f};  // Max interaction distance (talk, loot, etc.)

        // ===== VALIDATION & HELPER METHODS =====
        bool IsValid() const { return !guid.IsEmpty() && maxHealth > 0; }
        float GetHealthPercent() const { return maxHealth > 0 ? (static_cast<float>(health) / maxHealth) * 100.0f : 0.0f; }
        bool IsAlive() const { return !isDead && health > 0; }
        bool CanBeTargeted() const { return isAttackable && IsAlive() && !isUnkillable && isVisible; }  // Includes hostile AND neutral
        bool IsHostileTarget() const { return isHostile && CanBeTargeted(); }  // Only hostile (red) mobs
        bool IsInMeleeRange(Position const& targetPos) const
        {
            float dist = position.GetExactDist2d(targetPos);
            return dist <= (combatReach + boundingRadius);
        }
    };

    /**
     * @brief ENTERPRISE-GRADE Immutable snapshot of player data for thread-safe bot queries
     *
     * OPTION C: COMPLETE ENHANCEMENT - All 45+ fields from TrinityCore Player entity
     * Memory footprint: ~424 bytes per player (5000 players = 2.1 MB)
     */
    struct PlayerSnapshot
    {
        // ===== IDENTITY (CRITICAL) =====
        ObjectGuid guid;
        uint32 accountId{0};
        ::std::array<char, 48> name{};  // Fixed-size character name (no ::std::string for POD)

        // ===== POSITION & MOVEMENT (CRITICAL) =====
        Position position;
        uint32 mapId{0};
        uint32 instanceId{0};
        uint32 zoneId{0};
        uint32 areaId{0};
        uint32 displayId{0};
        uint32 mountDisplayId{0};
        bool isMounted{false};

        // ===== COMBAT & HEALTH (CRITICAL) =====
        uint64 health{0};
        uint64 maxHealth{0};
        uint8 powerType{0};  // POWER_MANA, POWER_RAGE, POWER_ENERGY, etc.
        int32 power{0};  // Current power (mana/rage/energy)
        int32 maxPower{0};
        bool isInCombat{false};
        ObjectGuid victim;  // Current combat target
        uint32 unitState{0};  // UNIT_STATE_* flags
        uint32 attackTimer{0};
        float combatReach{0.0f};

        // ===== CHARACTER STATS (CRITICAL/HIGH) =====
        uint8 level{0};
        uint32 experience{0};
        uint8 race{0};
        uint8 classId{0};
        uint8 gender{0};
        uint32 faction{0};

        // Primary stats
        float strength{0.0f};
        float agility{0.0f};
        float stamina{0.0f};
        float intellect{0.0f};
        float spirit{0.0f};  // WoW 12.0: Spirit stat re-added

        // ===== RESISTANCES & ARMOR (HIGH) =====
        int32 armor{0};
        int32 holyResist{0};
        int32 fireResist{0};
        int32 natureResist{0};
        int32 frostResist{0};
        int32 shadowResist{0};
        int32 arcaneResist{0};

        // ===== PLAYER FLAGS & STATUS (HIGH) =====
        uint32 playerFlags{0};  // PLAYER_FLAGS_* bitmask
        uint32 pvpFlags{0};  // PVP flags bitmask
        bool isAFK{false};
        bool isDND{false};
        bool isGhost{false};
        bool isResting{false};
        bool isPvP{false};
        bool isInPvPCombat{false};
        uint8 standState{0};

        // ===== SPECIALIZATION & TALENTS (HIGH) =====
        uint32 specialization{0};  // Primary spec ID
        uint8 activeSpec{0};  // Active spec index (0 or 1)
        uint32 talentCount{0};

        // ===== EQUIPMENT (HIGH) =====
        uint32 mainhandItem{0};
        uint32 offhandItem{0};
        uint32 rangedItem{0};
        uint32 headItem{0};
        uint32 chestItem{0};
        uint32 handsItem{0};

        // ===== MONEY & CURRENCY (MEDIUM) =====
        uint64 money{0};  // Gold in copper

        // ===== DEATH STATE (CRITICAL) =====
        bool isAlive{false};
        bool isDead{false};
        uint8 deathState{0};  // ALIVE, JUST_DIED, CORPSE, DEAD

        // ===== GROUP & SOCIAL (HIGH) =====
        ObjectGuid groupGuid;
        bool isGroupLeader{false};
        ObjectGuid guildGuid;

        // Movement flags (MEDIUM)
        bool isWalking{false};
        bool isHovering{false};
        bool isInWater{false};
        bool isUnderWater{false};
        bool isGravityDisabled{false};

        // ===== VALIDATION & HELPER METHODS =====
        bool IsValid() const { return !guid.IsEmpty(); }
        float GetHealthPercent() const { return maxHealth > 0 ? (static_cast<float>(health) / maxHealth) * 100.0f : 0.0f; }
        bool CanBeTargeted() const { return isAlive && !isGhost; }
    };

    /**
     * @brief ENTERPRISE-GRADE Immutable snapshot of GameObject data
     *
     * OPTION C: COMPLETE ENHANCEMENT - All 25+ fields
     * Memory footprint: ~160 bytes per GO
     */
    struct GameObjectSnapshot
    {
        // ===== IDENTITY (CRITICAL) =====
        ObjectGuid guid;
        uint32 entry{0};
        uint64 spawnId{0};

        // ===== POSITION (CRITICAL) =====
        Position position;
        uint32 mapId{0};
        uint32 instanceId{0};
        uint32 zoneId{0};
        uint32 areaId{0};
        uint32 displayId{0};

        // ===== TYPE & STATE (CRITICAL) =====
        uint8 goType{0};  // GAMEOBJECT_TYPE_DOOR, CHEST, BUTTON, etc.
        uint8 goState{0};  // GO_STATE_ACTIVE, GO_STATE_READY, etc.
        uint8 lootState{0};  // GO_NOT_READY, GO_READY, GO_ACTIVATED, GO_JUST_DEACTIVATED
        uint32 flags{0};
        uint32 level{0};
        uint8 animProgress{0};  // Animation state 0-100
        uint32 artKit{0};

        // ===== ROTATION (MEDIUM) =====
        float rotationX{0.0f};
        float rotationY{0.0f};
        float rotationZ{0.0f};
        float rotationW{1.0f};  // Quaternion (default identity)

        // ===== RESPAWN & LOOT (HIGH) =====
        time_t respawnTime{0};
        uint32 respawnDelay{0};
        bool isSpawned{false};
        bool spawnedByDefault{false};
        uint16 lootMode{0};
        uint32 spellId{0};
        ObjectGuid ownerGuid;
        uint32 faction{0};

        // Interaction
        float interactionRange{0.0f};
        bool isQuestObject{false};  // GameObject involved in quest (quest giver, objective, etc.)
        bool isUsable{false};  // Can be used/interacted with

        // ===== VALIDATION =====
        bool IsValid() const { return !guid.IsEmpty(); }
        bool CanInteract() const { return isSpawned && goState == 1; }  // GO_STATE_READY
    };

    /**
     * @brief ENTERPRISE-GRADE Immutable snapshot of AreaTrigger data
     *
     * OPTION C: COMPLETE ENHANCEMENT
     * Memory footprint: ~144 bytes per AreaTrigger
     */
    struct AreaTriggerSnapshot
    {
        // ===== IDENTITY (CRITICAL) =====
        ObjectGuid guid;
        uint32 entry{0};
        uint32 spellId{0};
        ObjectGuid casterGuid;
        ObjectGuid targetGuid;

        // ===== POSITION (CRITICAL) =====
        Position position;
        uint32 mapId{0};
        uint32 instanceId{0};
        uint32 zoneId{0};
        uint32 areaId{0};

        // ===== SHAPE (CRITICAL) =====
        uint8 shapeType{0};  // AREATRIGGER_SPHERE, BOX, POLYGON, CYLINDER, DISK, BOUNDED_PLANE
        float sphereRadius{0.0f};
        float boxExtentX{0.0f};
        float boxExtentY{0.0f};
        float boxExtentZ{0.0f};

        // ===== DURATION & MOVEMENT (HIGH) =====
        int32 duration{0};
        int32 totalDuration{0};
        uint32 flags{0};
        bool hasSplines{false};
        bool hasOrbit{false};
        bool isRemoved{false};
        float scale{1.0f};

        // ===== VALIDATION =====
        bool IsValid() const { return !guid.IsEmpty(); }
        bool IsActive() const { return !isRemoved && duration > 0; }
    };

    /**
     * @brief ENTERPRISE-GRADE Immutable snapshot of DynamicObject data
     *
     * OPTION C: COMPLETE ENHANCEMENT
     * Memory footprint: ~104 bytes per DynamicObject
     */
    struct DynamicObjectSnapshot
    {
        // ===== IDENTITY (CRITICAL) =====
        ObjectGuid guid;
        uint32 entry{0};
        uint32 spellId{0};
        ObjectGuid casterGuid;

        // ===== POSITION (CRITICAL) =====
        Position position;
        uint32 mapId{0};
        uint32 instanceId{0};
        uint32 zoneId{0};
        uint32 areaId{0};

        // ===== SPELL & EFFECT (CRITICAL) =====
        float radius{0.0f};
        int32 duration{0};  // Milliseconds remaining
        int32 totalDuration{0};  // Original duration
        uint8 type{0};  // DYNAMIC_OBJECT_PORTAL, DYNAMIC_OBJECT_AREA_SPELL, DYNAMIC_OBJECT_FARSIGHT
        uint32 faction{0};

        // Visual
        uint32 spellVisual{0};

        // ===== VALIDATION =====
        bool IsValid() const { return !guid.IsEmpty(); }
        bool IsActive() const { return duration > 0; }
    };

    /**
     * @brief Contents of a single cell
     * Stores immutable data snapshots instead of GUIDs - ZERO pointer dereferencing in worker threads!
     *
     * ENTERPRISE QUALITY: Complete data isolation ensures thread safety
     */
    struct CellContents
    {
        ::std::vector<CreatureSnapshot> creatures;
        ::std::vector<PlayerSnapshot> players;
        ::std::vector<GameObjectSnapshot> gameObjects;
        ::std::vector<AreaTriggerSnapshot> areaTriggers;
        ::std::vector<DynamicObjectSnapshot> dynamicObjects;

        void Clear()
        {
            creatures.clear();
            players.clear();
            gameObjects.clear();
            areaTriggers.clear();
            dynamicObjects.clear();
        }

        size_t GetTotalCount() const
        {
            return creatures.size() + players.size() + gameObjects.size() +
                   areaTriggers.size() + dynamicObjects.size();
        }

        // Memory estimation (for monitoring)
        size_t GetMemoryUsageBytes() const
        {
            return (creatures.size() * sizeof(CreatureSnapshot)) +
                   (players.size() * sizeof(PlayerSnapshot)) +
                   (gameObjects.size() * sizeof(GameObjectSnapshot));
        }
    };

    /**
     * @brief One complete spatial grid buffer - SPARSE STORAGE
     *
     * MEMORY OPTIMIZATION: Uses unordered_map instead of dense 512x512 array
     *
     * OLD (Dense):  512×512 = 262,144 cells always allocated = 31.46 MB per buffer
     * NEW (Sparse): Only populated cells allocated = typically 0.1-0.5 MB per buffer
     *
     * Trade-off: Slightly slower cell lookup (hash vs array index) but ~99% memory savings
     */
    struct GridBuffer
    {
        // SPARSE storage: Only cells with entities are stored
        ::std::unordered_map<CellKey, CellContents, CellKeyHash> cells;

        uint32 populationCount{0};
        uint32 activeCellCount{0};  // Number of cells with entities
        ::std::chrono::steady_clock::time_point lastUpdate;

        void Clear()
        {
            cells.clear();
            populationCount = 0;
            activeCellCount = 0;
        }

        // Get or create cell at coordinates (for write operations)
        CellContents& GetOrCreateCell(uint32 x, uint32 y)
        {
            CellKey key(x, y);
            auto it = cells.find(key);
            if (it == cells.end())
            {
                it = cells.emplace(key, CellContents{}).first;
                ++activeCellCount;
            }
            return it->second;
        }

        // Get cell at coordinates (for read operations, returns nullptr if not found)
        CellContents const* GetCell(uint32 x, uint32 y) const
        {
            CellKey key(x, y);
            auto it = cells.find(key);
            return (it != cells.end()) ? &it->second : nullptr;
        }

        // Calculate memory usage in bytes
        size_t GetMemoryUsageBytes() const
        {
            size_t total = sizeof(GridBuffer);
            // Hash map overhead: ~32 bytes per entry (key + value pointer + bucket)
            total += cells.size() * (sizeof(CellKey) + sizeof(CellContents) + 32);

            // Add snapshot memory
            for (auto const& [key, cell] : cells)
            {
                total += cell.GetMemoryUsageBytes();
                // Vector capacity overhead
                total += cell.areaTriggers.capacity() * sizeof(AreaTriggerSnapshot);
                total += cell.dynamicObjects.capacity() * sizeof(DynamicObjectSnapshot);
            }
            return total;
        }
    };

    /**
     * @brief Query statistics for monitoring - ENHANCED with memory tracking
     */
    struct Statistics
    {
        uint64_t totalQueries{0};
        uint64_t totalUpdates{0};
        uint64_t totalSwaps{0};
        uint32 lastUpdateDurationUs{0};
        uint32 currentPopulation{0};
        uint32 activeCellCount{0};      // NEW: Number of populated cells
        size_t memoryUsageBytes{0};     // NEW: Total memory usage
        size_t peakMemoryUsageBytes{0}; // NEW: Peak memory usage
        ::std::chrono::steady_clock::time_point startTime;

        // Helper for human-readable memory display
        float GetMemoryUsageMB() const { return static_cast<float>(memoryUsageBytes) / (1024.0f * 1024.0f); }
        float GetPeakMemoryUsageMB() const { return static_cast<float>(peakMemoryUsageBytes) / (1024.0f * 1024.0f); }
    };

    // Constructor/Destructor
    explicit DoubleBufferedSpatialGrid(Map* map);
    ~DoubleBufferedSpatialGrid();

    // Lifecycle
    void Start();  // DEPRECATED: No longer starts background thread
    void Stop();   // DEPRECATED: No-op now
    bool IsRunning() const { return true; }  // Always "running" now (no background thread)

    // Manual update (call from Map::Update or on-demand)
    // Const-qualified to allow calling from const query methods
    void Update() const;

    // Bot query interface (thread-safe, lock-free) - NOW RETURNS SNAPSHOTS!
    // BREAKING CHANGE: Methods now return data snapshots instead of GUIDs
    // This eliminates ALL ObjectAccessor/Map access from worker threads!
    ::std::vector<CreatureSnapshot> QueryNearbyCreatures(Position const& pos, float radius) const;
    ::std::vector<PlayerSnapshot> QueryNearbyPlayers(Position const& pos, float radius) const;
    ::std::vector<GameObjectSnapshot> QueryNearbyGameObjects(Position const& pos, float radius) const;
    ::std::vector<AreaTriggerSnapshot> QueryNearbyAreaTriggers(Position const& pos, float radius) const;
    ::std::vector<DynamicObjectSnapshot> QueryNearbyDynamicObjects(Position const& pos, float radius) const;

    // Legacy GUID-based queries (DEPRECATED - use snapshot queries instead!)
    // These are maintained temporarily for backward compatibility but will be removed
    ::std::vector<ObjectGuid> QueryNearbyCreatureGuids(Position const& pos, float radius) const;
    ::std::vector<ObjectGuid> QueryNearbyPlayerGuids(Position const& pos, float radius) const;
    ::std::vector<ObjectGuid> QueryNearbyGameObjectGuids(Position const& pos, float radius) const;

    // Cell-level query (for advanced usage)
    CellContents const& GetCell(uint32 x, uint32 y) const;

    // Statistics
    Statistics GetStatistics() const;
    uint32 GetPopulationCount() const;

    // Map pointer management (for SpatialGridManager)
    void SetMap(Map* map) { _map = map; }
    Map* GetMap() const { return _map; }

private:
    // Populate write buffer from Map entities
    void PopulateBufferFromMap();

    // Swap buffers atomically
    void SwapBuffers();

    // Helper: Get write buffer (inactive)
    GridBuffer& GetWriteBuffer()
    {
        return _buffers[1 - _readBufferIndex.load(::std::memory_order_relaxed)];
    }

    // Helper: Get read buffer (active)
    GridBuffer const& GetReadBuffer() const
    {
        return _buffers[_readBufferIndex.load(::std::memory_order_acquire)];
    }

    // Coordinate conversion helpers
    ::std::pair<uint32, uint32> GetCellCoords(Position const& pos) const;
    ::std::vector<::std::pair<uint32, uint32>> GetCellsInRadius(Position const& center, float radius) const;

    // Thread-safe Map entity access
    ::std::vector<Creature*> GetCreaturesInArea(Position const& center, float radius);
    ::std::vector<Player*> GetPlayersInArea(Position const& center, float radius);
    ::std::vector<GameObject*> GetGameObjectsInArea(Position const& center, float radius);
    ::std::vector<DynamicObject*> GetDynamicObjectsInArea(Position const& center, float radius);
    ::std::vector<AreaTrigger*> GetAreaTriggersInArea(Position const& center, float radius);

    // Check if update is needed (rate-limited)
    bool ShouldUpdate() const;

    // Data members
    Map* _map;
    uint32 _mapId;  // Cached for safe logging (doesn't change during grid lifetime)
    mutable GridBuffer _buffers[2];  // Mutable to allow updates from const methods
    mutable ::std::atomic<uint32> _readBufferIndex{0};

    mutable ::std::chrono::steady_clock::time_point _lastUpdate;
    mutable OrderedMutex<LockOrder::SPATIAL_GRID> _updateMutex;  // Protects Update() to ensure only one thread updates at a time

    // Statistics (atomic for thread-safe access)
    mutable ::std::atomic<uint64_t> _totalQueries{0};
    mutable ::std::atomic<uint64_t> _totalUpdates{0};
    mutable ::std::atomic<uint64_t> _totalSwaps{0};
    mutable ::std::atomic<uint32> _lastUpdateDurationUs{0};

    // Memory tracking
    mutable ::std::atomic<size_t> _currentMemoryUsage{0};
    mutable ::std::atomic<size_t> _peakMemoryUsage{0};

    ::std::chrono::steady_clock::time_point _startTime;

    // Non-copyable, non-movable
    DoubleBufferedSpatialGrid(DoubleBufferedSpatialGrid const&) = delete;
    DoubleBufferedSpatialGrid& operator=(DoubleBufferedSpatialGrid const&) = delete;
    DoubleBufferedSpatialGrid(DoubleBufferedSpatialGrid&&) = delete;
    DoubleBufferedSpatialGrid& operator=(DoubleBufferedSpatialGrid&&) = delete;
};

} // namespace Playerbot

#endif // PLAYERBOT_DOUBLE_BUFFERED_SPATIAL_GRID_H
