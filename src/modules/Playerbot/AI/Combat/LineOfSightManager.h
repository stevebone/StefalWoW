/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Define.h"
#include "Threading/LockHierarchy.h"
#include "ObjectGuid.h"
#include "Position.h"
#include "Core/LRUCache.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <atomic>
#include <shared_mutex>
#include <chrono>

// Forward declarations
class Player;
class Unit;
class WorldObject;
class Map;
class GameObject;

namespace Playerbot
{

// Line of sight check types
enum class LoSCheckType : uint8
{
    BASIC = 0,              // Simple visibility check
    SPELL_CASTING = 1,      // For spell target validation
    RANGED_COMBAT = 2,      // For ranged attacks
    MOVEMENT = 3,           // For pathfinding
    HEALING = 4,            // For healing spells
    INTERRUPT = 5,          // For spell interrupts
    AREA_CHECK = 6,         // For area of effect validation
    OBSTACLE_DETECTION = 7   // For obstacle avoidance
};

// Line of sight validation flags
enum class LoSValidation : uint32
{
    NONE = 0x00000000,
    TERRAIN = 0x00000001,           // Check terrain blocking
    BUILDINGS = 0x00000002,         // Check building/structure blocking
    OBJECTS = 0x00000004,           // Check object blocking
    UNITS = 0x00000008,             // Check unit blocking
    WATER = 0x00000010,             // Check water surfaces
    DYNAMIC = 0x00000020,           // Check dynamic objects (doors, etc.)
    HEIGHT_CHECK = 0x00000040,      // Check height differences
    ANGLE_CHECK = 0x00000080,       // Check viewing angles
    RANGE_CHECK = 0x00000100,       // Check maximum range
    SPELL_SPECIFIC = 0x00000200,    // Spell-specific LoS rules

    // Common validation combinations
    BASIC_LOS = TERRAIN | BUILDINGS | HEIGHT_CHECK,
    COMBAT_LOS = BASIC_LOS | OBJECTS | RANGE_CHECK,
    SPELL_LOS = COMBAT_LOS | SPELL_SPECIFIC | ANGLE_CHECK,
    MOVEMENT_LOS = TERRAIN | BUILDINGS | OBJECTS | WATER
};

DEFINE_ENUM_FLAG(LoSValidation);

// Line of sight result information
struct LoSResult
{
    bool hasLineOfSight;
    float distance;
    float heightDifference;
    Position blockingPoint;
    ObjectGuid blockingObjectGuid;
    ::std::string blockingObjectName;
    LoSCheckType checkType;
    uint32 checkTime;
    ::std::string failureReason;

    // Detailed obstruction information
    bool blockedByTerrain;
    bool blockedByBuilding;
    bool blockedByObject;
    bool blockedByUnit;
    bool blockedByWater;
    bool blockedByHeight;
    bool blockedByAngle;
    bool blockedByRange;

    LoSResult() : hasLineOfSight(false), distance(0.0f), heightDifference(0.0f),
                 checkType(LoSCheckType::BASIC), checkTime(0),
                 blockedByTerrain(false), blockedByBuilding(false), blockedByObject(false),
                 blockedByUnit(false), blockedByWater(false), blockedByHeight(false),
                 blockedByAngle(false), blockedByRange(false) {}
};

// Line of sight cache entry
struct LoSCacheEntry
{
    ObjectGuid sourceGuid;
    ObjectGuid targetGuid;
    LoSResult result;
    uint32 timestamp;
    uint32 expirationTime;
    LoSCheckType checkType;

    bool IsExpired(uint32 currentTime) const
    {
        return currentTime > expirationTime;
    }

    bool IsValid(uint32 currentTime) const
    {
        return !IsExpired(currentTime);
    }
};

// Optimized cache key structure (eliminates string allocations)
struct LoSCacheKey
{
    uint64 sourceGuidLow;
    uint64 sourceGuidHigh;
    uint64 targetGuidLow;
    uint64 targetGuidHigh;
    uint8 checkType;

    LoSCacheKey(ObjectGuid source, ObjectGuid target, LoSCheckType type)
        : sourceGuidLow(source.GetRawValue(0))
        , sourceGuidHigh(source.GetRawValue(1))
        , targetGuidLow(target.GetRawValue(0))
        , targetGuidHigh(target.GetRawValue(1))
        , checkType(static_cast<uint8>(type))
    {}

    bool operator==(const LoSCacheKey& other) const
    {
        return sourceGuidLow == other.sourceGuidLow &&
               sourceGuidHigh == other.sourceGuidHigh &&
               targetGuidLow == other.targetGuidLow &&
               targetGuidHigh == other.targetGuidHigh &&
               checkType == other.checkType;
    }
};

// Hash function for LoSCacheKey
struct LoSCacheKeyHash
{
    ::std::size_t operator()(const LoSCacheKey& key) const noexcept
    {
        // FNV-1a hash algorithm for combining five values
        ::std::size_t hash = 14695981039346656037ULL;
        hash ^= key.sourceGuidLow;
        hash *= 1099511628211ULL;
        hash ^= key.sourceGuidHigh;
        hash *= 1099511628211ULL;
        hash ^= key.targetGuidLow;
        hash *= 1099511628211ULL;
        hash ^= key.targetGuidHigh;
        hash *= 1099511628211ULL;
        hash ^= key.checkType;
        hash *= 1099511628211ULL;
        return hash;
    }
};

// Line of sight context for checks
struct LoSContext
{
    Player* bot;
    Unit* source;
    Unit* target;
    Position sourcePos;
    Position targetPos;
    LoSCheckType checkType;
    LoSValidation validationFlags;
    float maxRange;
    float maxHeightDiff;
    float viewAngleTolerance;
    uint32 spellId;
    bool ignoreUnits;
    bool allowPartialBlocking;

    LoSContext() : bot(nullptr), source(nullptr), target(nullptr),
                  checkType(LoSCheckType::BASIC), validationFlags(LoSValidation::BASIC_LOS),
                  maxRange(100.0f), maxHeightDiff(50.0f), viewAngleTolerance(static_cast<float>(M_PI) / 3.0f),
                  spellId(0), ignoreUnits(false), allowPartialBlocking(false) {}
};

// Performance metrics for LoS system
struct LoSMetrics
{
    ::std::atomic<uint32> totalChecks{0};
    ::std::atomic<uint32> cacheHits{0};
    ::std::atomic<uint32> cacheMisses{0};
    ::std::atomic<uint32> successfulChecks{0};
    ::std::atomic<uint32> failedChecks{0};
    ::std::chrono::microseconds averageCheckTime{0};
    ::std::chrono::microseconds maxCheckTime{0};
    ::std::chrono::steady_clock::time_point lastUpdate;

    void Reset()
    {
        totalChecks = 0;
        cacheHits = 0;
        cacheMisses = 0;
        successfulChecks = 0;
        failedChecks = 0;
        averageCheckTime = ::std::chrono::microseconds{0};
        maxCheckTime = ::std::chrono::microseconds{0};
        lastUpdate = ::std::chrono::steady_clock::now();
    }

    float GetCacheHitRate() const
    {
        uint32 total = totalChecks.load();
        return total > 0 ? static_cast<float>(cacheHits.load()) / total : 0.0f;
    }

    float GetSuccessRate() const
    {
        uint32 total = totalChecks.load();
        return total > 0 ? static_cast<float>(successfulChecks.load()) / total : 0.0f;
    }
};

class TC_GAME_API LineOfSightManager
{
public:
    explicit LineOfSightManager(Player* bot);
    ~LineOfSightManager() = default;

    // Primary LoS checking interface
    LoSResult CheckLineOfSight(const LoSContext& context);
    LoSResult CheckLineOfSight(Unit* target, LoSCheckType checkType = LoSCheckType::BASIC);
    LoSResult CheckLineOfSight(const Position& targetPos, LoSCheckType checkType = LoSCheckType::BASIC);
    LoSResult CheckSpellLineOfSight(Unit* target, uint32 spellId);

    // Specific LoS check types
    bool CanSeeTarget(Unit* target);
    bool CanCastSpell(uint32 spellId, Unit* target);
    bool CanAttackTarget(Unit* target);
    bool CanHealTarget(Unit* target);
    bool CanInterruptTarget(Unit* target);
    bool CanMoveToPosition(const Position& pos);

    // Advanced LoS analysis
    ::std::vector<Position> FindLineOfSightPositions(Unit* target, float radius = 10.0f);
    Position FindBestLineOfSightPosition(Unit* target, float preferredRange = 0.0f);
    bool HasLineOfSightFromPosition(const Position& fromPos, Unit* target);
    bool WillHaveLineOfSightAfterMovement(const Position& newPos, Unit* target);

    // Obstruction analysis
    ::std::vector<ObjectGuid> GetBlockingObjects(Unit* target);
    Position GetClosestUnblockedPosition(Unit* target);
    bool IsObstructionTemporary(const LoSResult& result);
    float EstimateTimeUntilClearPath(Unit* target);

    // Angle and positioning
    bool IsWithinViewingAngle(Unit* target, float maxAngle = static_cast<float>(M_PI) / 3.0f);
    float CalculateViewingAngle(Unit* target);
    bool RequiresFacing(Unit* target, LoSCheckType checkType);
    Position CalculateOptimalViewingPosition(Unit* target);

    // Multi-target LoS management
    ::std::vector<Unit*> GetVisibleEnemies(float maxRange = 40.0f);
    ::std::vector<Unit*> GetVisibleAllies(float maxRange = 40.0f);
    Unit* GetBestVisibleTarget(const ::std::vector<Unit*>& candidates);
    uint32 CountVisibleTargets(const ::std::vector<Unit*>& targets);

    // Height and elevation handling
    bool IsHeightDifferenceBlocking(Unit* target);
    float CalculateHeightAdvantage(Unit* target);
    bool HasElevationAdvantage(Unit* target);
    Position FindElevatedPosition(Unit* target);

    // Dynamic obstruction handling
    void UpdateDynamicObstructions();
    void RegisterDynamicObstruction(GameObject* obj);
    void UnregisterDynamicObstruction(GameObject* obj);
    bool IsDynamicObstructionActive(ObjectGuid guid);

    // Cache management
    void ClearCache();
    void ClearExpiredCacheEntries();
    void SetCacheDuration(uint32 durationMs) { _losCache.SetTTL(::std::chrono::milliseconds(durationMs)); }
    uint32 GetCacheDuration() const { return static_cast<uint32>(_losCache.GetTTL().count()); }

    // Performance monitoring
    LoSMetrics const& GetMetrics() const { return _metrics; }
    void ResetMetrics() { _metrics.Reset(); }
    void EnableProfiling(bool enable) { _profilingEnabled = enable; }

    // Configuration
    void SetMaxRange(float range) { _maxRange = range; }
    float GetMaxRange() const { return _maxRange; }
    void SetHeightTolerance(float tolerance) { _heightTolerance = tolerance; }
    float GetHeightTolerance() const { return _heightTolerance; }

    // Spell-specific LoS handling
    bool HasSpellLineOfSight(Unit* target, uint32 spellId);
    float GetSpellMaxRange(uint32 spellId);
    bool IsSpellRangeBlocked(Unit* target, uint32 spellId);
    LoSValidation GetSpellLoSRequirements(uint32 spellId);

    // Area of effect LoS
    bool CanCastAoEAtPosition(const Position& targetPos, uint32 spellId);
    ::std::vector<Unit*> GetAoETargetsInLoS(const Position& centerPos, float radius);
    bool IsAoEPositionOptimal(const Position& pos, const ::std::vector<Unit*>& targets);

    // Movement and pathfinding integration
    bool IsPathClear(const ::std::vector<Position>& waypoints);
    Position GetFirstBlockedWaypoint(const ::std::vector<Position>& waypoints);
    bool CanSeeDestination(const Position& destination);
    ::std::vector<Position> GetVisibilityWaypoints(const Position& destination);

private:
    // Core LoS calculation methods
    LoSResult PerformLineOfSightCheck(const LoSContext& context);
    bool CheckTerrainBlocking(const Position& from, const Position& to);
    bool CheckBuildingBlocking(const Position& from, const Position& to);
    bool CheckObjectBlocking(const Position& from, const Position& to);
    bool CheckUnitBlocking(const Position& from, const Position& to, Unit* ignoreUnit = nullptr);
    bool CheckWaterBlocking(const Position& from, const Position& to);

    // Cache management methods
    void AddCacheEntry(const LoSCacheEntry& entry);
    void CleanupCache();

    // Utility methods
    float CalculateDistance3D(const Position& from, const Position& to);
    bool IsWithinRange(const Position& from, const Position& to, float maxRange);
    bool IsHeightDifferenceAcceptable(const Position& from, const Position& to, float maxDiff);
    bool IsAngleAcceptable(const Position& from, const Position& to, float maxAngle);

    // Map and world interaction
    bool IsPositionInWorld(const Position& pos);
    bool IsPositionAccessible(const Position& pos);
    float GetGroundLevel(const Position& pos);
    bool IsUnderground(const Position& pos);

    // Performance tracking
    void TrackPerformance(::std::chrono::microseconds duration, bool cacheHit, bool successful);
    void UpdateMetrics();

    // Specialized LoS checks
    bool CheckSpellSpecificRequirements(Unit* target, uint32 spellId);
    bool CheckInterruptLineOfSight(Unit* target);
    bool CheckHealingLineOfSight(Unit* target);
    bool CheckRangedCombatLineOfSight(Unit* target);

private:
    Player* _bot;

    // Cache system - NOW USES LRU CACHE for automatic memory management
    // (using struct key eliminates string allocations, combined with LRU for eviction)
    LRUCache<LoSCacheKey, LoSCacheEntry, LoSCacheKeyHash> _losCache{
        MAX_CACHE_SIZE,
        ::std::chrono::milliseconds(DEFAULT_CACHE_DURATION)
    };

    // Configuration
    float _maxRange;
    float _heightTolerance;
    float _angleTolerance;
    bool _enableCaching;
    bool _profilingEnabled;

    // Dynamic obstruction tracking - bounded to prevent unbounded growth
    BoundedMap<ObjectGuid, GameObject*> _dynamicObstructions{MAX_DYNAMIC_OBSTRUCTIONS};
    uint32 _lastObstructionUpdate;

    // Performance metrics
    mutable LoSMetrics _metrics;

    // Thread safety
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE> _mutex;

    // Constants
    static constexpr uint32 DEFAULT_CACHE_DURATION = 1000;       // 1 second
    static constexpr float DEFAULT_MAX_RANGE = 100.0f;           // 100 yards
    static constexpr float DEFAULT_HEIGHT_TOLERANCE = 10.0f;     // 10 yards
    static constexpr uint32 CACHE_CLEANUP_INTERVAL = 5000;       // 5 seconds
    static constexpr uint32 OBSTRUCTION_UPDATE_INTERVAL = 2000;  // 2 seconds
    static constexpr size_t MAX_CACHE_SIZE = 1000;               // Maximum LOS cache entries
    static constexpr size_t MAX_DYNAMIC_OBSTRUCTIONS = 100;      // Maximum tracked obstructions
};

// Line of sight utilities
class TC_GAME_API LoSUtils
{
public:
    // Static convenience methods
    static bool HasLoS(Player* source, Unit* target);
    static bool HasLoS(const Position& from, const Position& to, Map* map);
    static float GetLoSDistance(Player* source, Unit* target);
    static bool IsLoSBlocked(Player* source, Unit* target, ::std::string& reason);

    // Geometric utilities
    static bool IsPointBehindPoint(const Position& observer, const Position& target, const Position& reference);
    static Position GetLineIntersection(const Position& line1Start, const Position& line1End,
                                      const Position& line2Start, const Position& line2End);
    static bool DoLinesIntersect(const Position& line1Start, const Position& line1End,
                               const Position& line2Start, const Position& line2End);

    // Height and elevation utilities
    static bool IsAboveGround(const Position& pos, Map* map, float threshold = 5.0f);
    static bool IsBelowGround(const Position& pos, Map* map, float threshold = 2.0f);
    static float GetVerticalClearance(const Position& pos, Map* map);

    // Spell-specific utilities
    static bool CanCastSpellAtTarget(Player* caster, Unit* target, uint32 spellId);
    static bool CanCastSpellAtPosition(Player* caster, const Position& pos, uint32 spellId);
    static float GetEffectiveSpellRange(Player* caster, uint32 spellId);

    // Area validation utilities
    static bool IsAreaClear(const Position& center, float radius, Map* map);
    static ::std::vector<Position> GetBlockedPositionsInArea(const Position& center, float radius, Map* map);
    static Position GetClearedPositionNear(const Position& target, float searchRadius, Map* map);

    // Pathfinding integration utilities
    static bool IsDirectPathClear(const Position& from, const Position& to, Map* map);
    static ::std::vector<Position> GetLoSBreakpoints(const Position& from, const Position& to, Map* map);
    static Position GetLastVisiblePoint(const Position& from, const Position& to, Map* map);
};

} // namespace Playerbot