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
#include "EnumFlag.h"
#include "ObjectGuid.h"
#include "Position.h"
#include "SharedDefines.h"
#include <vector>
#include <unordered_map>
#include <memory>
#include <chrono>
#include <optional>
#include <atomic>
#include <shared_mutex>

// Forward declarations
class Player;
class Unit;
class Spell;
class SpellInfo;
class WorldObject;
class AreaTrigger;

namespace Playerbot
{

// Types of mechanics to detect and handle
enum class MechanicType : uint32
{
    NONE = 0x00000000,
    AOE_DAMAGE = 0x00000001,           // Area damage effects
    FRONTAL_CLEAVE = 0x00000002,       // Frontal cone attacks
    TAIL_SWIPE = 0x00000004,           // Tail/rear attacks
    WHIRLWIND = 0x00000008,            // 360-degree attacks
    CHARGE = 0x00000010,               // Charge mechanics
    KNOCKBACK = 0x00000020,            // Knockback effects
    PULL = 0x00000040,                 // Pull/grip mechanics
    FEAR = 0x00000080,                 // Fear effects
    STUN = 0x00000100,                 // Stun mechanics
    ROOT = 0x00000200,                 // Root/snare effects
    SILENCE = 0x00000400,              // Silence zones
    DISPEL_REQUIRED = 0x00000800,      // Needs dispel
    INTERRUPT_REQUIRED = 0x00001000,   // Needs interrupt
    STACK_REQUIRED = 0x00002000,       // Stack for shared damage
    SPREAD_REQUIRED = 0x00004000,      // Spread to avoid chains
    SOAK_REQUIRED = 0x00008000,        // Requires soaking
    TANK_SWAP = 0x00010000,           // Tank swap mechanic
    POSITIONAL = 0x00020000,          // Positional requirement
    MOVEMENT_REQUIRED = 0x00040000,   // Requires movement
    LOS_BREAK = 0x00080000,           // Break line of sight
    GROUND_EFFECT = 0x00100000,       // Ground-based effect
    PROJECTILE = 0x00200000,          // Projectile to dodge
    DEBUFF_SPREAD = 0x00400000,       // Spreading debuff
    HEAL_ABSORB = 0x00800000,         // Healing absorption
    DAMAGE_SHARE = 0x01000000,        // Damage sharing mechanic
    REFLECT = 0x02000000,             // Spell reflection
    ENRAGE = 0x04000000,              // Enrage mechanic
    PHASE_CHANGE = 0x08000000,        // Phase transition
    ADD_SPAWN = 0x10000000,           // Adds spawning
    ENVIRONMENTAL = 0x20000000,       // Environmental hazard

    // Common combinations
    CLEAVE_MECHANICS = FRONTAL_CLEAVE | TAIL_SWIPE | WHIRLWIND,
    MOVEMENT_MECHANICS = CHARGE | KNOCKBACK | PULL | MOVEMENT_REQUIRED,
    CONTROL_MECHANICS = FEAR | STUN | ROOT | SILENCE,
    RAID_MECHANICS = STACK_REQUIRED | SPREAD_REQUIRED | SOAK_REQUIRED | DAMAGE_SHARE
};

DEFINE_ENUM_FLAG(MechanicType);

// Mechanic urgency levels
enum class MechanicUrgency : uint8
{
    IMMEDIATE = 0,      // React within 500ms
    URGENT = 1,         // React within 1 second
    HIGH = 2,           // React within 2 seconds
    MODERATE = 3,       // React within 3 seconds
    LOW = 4,            // React within 5 seconds
    PASSIVE = 5         // No immediate reaction needed
};

// Response actions to mechanics
enum class MechanicResponse : uint8
{
    NONE = 0,
    MOVE_AWAY = 1,          // Move away from danger
    MOVE_TO = 2,            // Move to specific position
    SPREAD_OUT = 3,         // Spread from others
    STACK_UP = 4,           // Stack with group
    INTERRUPT = 5,          // Interrupt cast
    DISPEL = 6,             // Dispel effect
    USE_DEFENSIVE = 7,      // Use defensive cooldown
    USE_IMMUNITY = 8,       // Use immunity
    BREAK_LOS = 9,          // Break line of sight
    STOP_CASTING = 10,      // Stop current cast
    FACE_AWAY = 11,         // Turn away from source
    SOAK = 12,              // Soak mechanic
    AVOID = 13,             // Avoid area
    TANK_SWAP = 14,         // Execute tank swap
    HEAL_PRIORITY = 15      // Priority healing needed
};

// Mechanic detection result
struct MechanicInfo
{
    MechanicType type = MechanicType::NONE;
    MechanicUrgency urgency = MechanicUrgency::PASSIVE;
    MechanicResponse response = MechanicResponse::NONE;
    Position sourcePosition;
    Position safePosition;
    float dangerRadius = 0.0f;
    float safeDistance = 0.0f;
    uint32 spellId = 0;
    ObjectGuid sourceGuid;
    ObjectGuid targetGuid;
    uint32 triggerTime = 0;
    uint32 duration = 0;
    float damageEstimate = 0.0f;
    bool isActive = false;
    bool requiresGroupResponse = false;
    ::std::string description;

    bool IsExpired(uint32 currentTime) const
    {
        return duration > 0 && currentTime > triggerTime + duration;
    }

    bool RequiresImmediateAction() const
    {
        return urgency <= MechanicUrgency::URGENT && isActive;
    }
};

// AOE zone information
struct AOEZone
{
    Position center;
    float radius = 0.0f;
    float angle = 360.0f;          // For cone effects
    float orientation = 0.0f;      // Direction for cones
    MechanicType type = MechanicType::AOE_DAMAGE;
    uint32 spellId = 0;
    ObjectGuid casterGuid;
    uint32 startTime = 0;
    uint32 duration = 0;
    float damagePerTick = 0.0f;
    uint32 tickInterval = 1000;
    bool isPersistent = false;
    bool isGrowing = false;
    float growthRate = 0.0f;
    bool requiresSoak = false;
    uint32 soakCount = 0;

    bool IsPointInZone(const Position& point, uint32 currentTime = 0) const;
    float GetCurrentRadius(uint32 currentTime = 0) const;
    bool IsActive(uint32 currentTime) const;
    float EstimateDamage(uint32 timeInZone) const;
};

// Projectile tracking
struct ProjectileInfo
{
    Position origin;
    Position destination;
    Position currentPosition;
    float speed = 0.0f;
    float radius = 0.0f;
    uint32 spellId = 0;
    ObjectGuid casterGuid;
    ObjectGuid targetGuid;
    uint32 launchTime = 0;
    uint32 impactTime = 0;
    bool isTracking = false;
    bool isPiercing = false;
    float damage = 0.0f;

    Position PredictPosition(uint32 atTime) const;
    bool WillHitPosition(const Position& pos, float tolerance = 2.0f) const;
    uint32 TimeToImpact(uint32 currentTime) const;
};

// Cleave mechanic details
struct CleaveMechanic
{
    Unit* source = nullptr;
    float angle = 90.0f;           // Cone angle
    float range = 10.0f;           // Cleave range
    float damage = 0.0f;
    bool isActive = false;
    uint32 nextCleaveTime = 0;
    uint32 cleaveInterval = 0;
    bool isPredictable = false;

    bool IsPositionSafe(const Position& pos) const;
    float GetSafeAngle(bool preferLeft = true) const;
};

// Safe position calculation result
struct SafePositionResult
{
    Position position;
    float safetyScore = 0.0f;
    float distanceToMove = 0.0f;
    MechanicResponse requiredResponse = MechanicResponse::NONE;
    bool requiresMovement = false;
    bool isOptimal = false;
    ::std::vector<Position> alternativePositions;
    ::std::string reasoning;
};

// Mechanic prediction
struct MechanicPrediction
{
    MechanicType type = MechanicType::NONE;
    uint32 predictedTime = 0;
    float confidence = 0.0f;
    Position predictedLocation;
    float predictedRadius = 0.0f;
    ::std::string basis;  // What the prediction is based on
};

// Performance metrics for mechanic handling
struct MechanicMetrics
{
    ::std::atomic<uint32> mechanicsDetected{0};
    ::std::atomic<uint32> mechanicsAvoided{0};
    ::std::atomic<uint32> mechanicsFailed{0};
    ::std::atomic<uint32> falsePositives{0};
    ::std::atomic<uint32> reactionTimeTotal{0};
    ::std::atomic<uint32> reactionCount{0};
    ::std::chrono::steady_clock::time_point lastUpdate;

    float GetAverageReactionTime() const
    {
        return reactionCount > 0 ? reactionTimeTotal.load() / float(reactionCount.load()) : 0.0f;
    }

    float GetSuccessRate() const
    {
        uint32 total = mechanicsDetected.load();
        return total > 0 ? mechanicsAvoided.load() / float(total) * 100.0f : 0.0f;
    }
};

// Main mechanic awareness system
class TC_GAME_API MechanicAwareness
{
public:
    MechanicAwareness();
    ~MechanicAwareness() = default;

    // Mechanic detection
    ::std::vector<MechanicInfo> DetectMechanics(Player* bot, Unit* target);
    MechanicInfo AnalyzeSpellMechanic(uint32 spellId, Unit* caster, Unit* target = nullptr);
    bool DetectAOECast(Unit* caster, float& radius, Position& center);
    bool DetectCleave(Unit* target, float& angle, float& range);

    // Specific mechanic handlers
    void HandleCleaveMechanic(Unit* target, float cleaveAngle, float cleaveRange);
    void HandleAOEMechanic(const AOEZone& zone, Player* bot);
    void HandleProjectile(const ProjectileInfo& projectile, Player* bot);
    void HandleGroundEffect(const Position& center, float radius, Player* bot);

    // Safe position calculation
    SafePositionResult CalculateSafePosition(Player* bot, const ::std::vector<MechanicInfo>& threats);
    Position FindSafeSpot(Player* bot, const AOEZone& danger, float minSafeDistance = 5.0f);
    ::std::vector<Position> GenerateSafePositions(const Position& currentPos, float searchRadius = 20.0f);
    bool IsPositionSafe(const Position& pos, const ::std::vector<AOEZone>& dangers, uint32 currentTime = 0);

    // Response determination
    MechanicResponse DetermineResponse(Player* bot, const MechanicInfo& mechanic);
    void RespondToPositionalRequirement(Spell* spell, Player* caster);
    bool ShouldInterrupt(Unit* target, uint32 spellId);
    bool ShouldDispel(Unit* target, uint32 spellId);

    // AOE zone management
    void RegisterAOEZone(const AOEZone& zone);
    void UpdateAOEZones(uint32 currentTime);
    void RemoveExpiredZones(uint32 currentTime);
    ::std::vector<AOEZone> GetActiveAOEZones() const;
    ::std::vector<AOEZone> GetUpcomingAOEZones(uint32 timeWindow = 3000) const;

    // Projectile tracking
    void TrackProjectile(const ProjectileInfo& projectile);
    void UpdateProjectiles(uint32 currentTime);
    ::std::vector<ProjectileInfo> GetIncomingProjectiles(Player* target) const;
    bool WillProjectileHit(const ProjectileInfo& projectile, Player* target, float tolerance = 2.0f);

    // Cleave tracking
    void RegisterCleaveMechanic(Unit* source, const CleaveMechanic& cleave);
    void UpdateCleaveMechanics();
    bool IsInCleaveZone(Player* bot, Unit* source);
    Position GetCleaveAvoidancePosition(Player* bot, Unit* source);

    // Mechanic prediction
    ::std::vector<MechanicPrediction> PredictMechanics(Unit* target, uint32 timeAhead = 5000);
    MechanicPrediction PredictNextMechanic(Unit* target);
    float CalculateMechanicProbability(Unit* target, MechanicType type);

    // Group coordination
    void CoordinateGroupResponse(const MechanicInfo& mechanic, ::std::vector<Player*> group);
    ::std::unordered_map<ObjectGuid, Position> CalculateSpreadPositions(const ::std::vector<Player*>& group, float minDistance = 8.0f);
    Position CalculateStackPosition(const ::std::vector<Player*>& group);

    // Spell database integration
    bool IsInterruptibleSpell(uint32 spellId);
    bool IsDispellableDebuff(uint32 spellId);
    bool RequiresSoak(uint32 spellId);
    float GetSpellDangerRadius(uint32 spellId);

    // Environmental hazards
    void RegisterEnvironmentalHazard(const Position& location, float radius, uint32 duration);
    bool IsEnvironmentalHazard(const Position& pos);
    ::std::vector<Position> GetEnvironmentalHazards() const;

    // Performance and configuration
    void SetReactionTime(uint32 minMs, uint32 maxMs);
    void SetDangerThreshold(float threshold) { _dangerThreshold = threshold; }
    float GetDangerThreshold() const { return _dangerThreshold; }

    // Metrics
    MechanicMetrics const& GetMetrics() const { return _metrics; }
    void ResetMetrics()
    {
        _metrics.mechanicsDetected.store(0);
        _metrics.mechanicsAvoided.store(0);
        _metrics.mechanicsFailed.store(0);
        _metrics.falsePositives.store(0);
        _metrics.reactionTimeTotal.store(0);
        _metrics.reactionCount.store(0);
        _metrics.lastUpdate = ::std::chrono::steady_clock::now();
    }

    // Debugging and logging
    void LogMechanicDetection(const MechanicInfo& mechanic);
    void LogMechanicResponse(Player* bot, const MechanicInfo& mechanic, MechanicResponse response);

    // Static utilities
    static bool IsDangerousSpell(uint32 spellId);
    static float EstimateSpellDamage(uint32 spellId, Unit* caster, Unit* target);
    static bool RequiresPositioning(uint32 spellId);
    static MechanicType GetSpellMechanicType(uint32 spellId);

private:
    // Internal detection methods
    MechanicInfo DetectCastingMechanic(Unit* caster);
    MechanicInfo DetectAreaTrigger(AreaTrigger* trigger);
    MechanicInfo DetectDebuffMechanic(Player* bot);
    ::std::vector<MechanicInfo> ScanForThreats(Player* bot, float scanRadius = 50.0f);

    // Position evaluation
    float EvaluatePositionSafety(const Position& pos, const ::std::vector<MechanicInfo>& threats);
    float CalculateDangerScore(const Position& pos, const AOEZone& zone, uint32 currentTime);
    bool ValidateSafePosition(const Position& pos, Player* bot);

    // Response execution helpers
    void ExecuteMovementResponse(Player* bot, const Position& safePos, MechanicUrgency urgency);
    void ExecuteDefensiveResponse(Player* bot, const MechanicInfo& mechanic);
    void ExecuteGroupResponse(const ::std::vector<Player*>& group, MechanicResponse response);

    // Prediction helpers
    void UpdateMechanicHistory(Unit* target, const MechanicInfo& mechanic);
    float AnalyzeMechanicPattern(Unit* target, MechanicType type);

    // Data management
    void CleanupOldData(uint32 currentTime);
    void MergeOverlappingZones();

private:
    // Active tracking
    ::std::vector<AOEZone> _activeAOEZones;
    ::std::vector<ProjectileInfo> _trackedProjectiles;
    ::std::unordered_map<ObjectGuid, CleaveMechanic> _cleaveMechanics;
    ::std::vector<Position> _environmentalHazards;

    // Mechanic history for prediction
    ::std::unordered_map<ObjectGuid, ::std::vector<MechanicInfo>> _mechanicHistory;
    ::std::unordered_map<uint32, MechanicType> _spellMechanicCache;

    // Configuration
    uint32 _minReactionTime = 200;    // Minimum 200ms reaction time
    uint32 _maxReactionTime = 500;    // Maximum 500ms reaction time
    float _dangerThreshold = 0.7f;    // 70% danger threshold
    float _safeDistanceBuffer = 3.0f; // Extra safety distance
    uint32 _maxHistorySize = 100;     // Maximum mechanic history entries
    uint32 _predictionWindow = 5000;  // 5 second prediction window

    // Performance metrics
    mutable MechanicMetrics _metrics;

    // Thread safety
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE> _mutex;

    // Constants
    static constexpr float DEFAULT_AOE_RADIUS = 8.0f;
    static constexpr float DEFAULT_CLEAVE_ANGLE = 90.0f;
    static constexpr float DEFAULT_SAFE_DISTANCE = 10.0f;
    static constexpr uint32 ZONE_CLEANUP_INTERVAL = 1000;
    static constexpr float PROJECTILE_HIT_TOLERANCE = 2.0f;
};

// Global mechanic database (singleton pattern for spell mechanic data)
class TC_GAME_API MechanicDatabase
{
public:
    static MechanicDatabase& Instance();

    // Spell mechanic registration
    void RegisterSpellMechanic(uint32 spellId, MechanicType type, float radius = 0.0f, float angle = 0.0f);
    MechanicType GetSpellMechanicType(uint32 spellId) const;
    float GetSpellRadius(uint32 spellId) const;
    float GetSpellAngle(uint32 spellId) const;

    // Load WoW 12.0 mechanic data
    void LoadMechanicData();
    void LoadDungeonMechanics();
    void LoadRaidMechanics();

private:
    MechanicDatabase() = default;
    ~MechanicDatabase() = default;
    MechanicDatabase(const MechanicDatabase&) = delete;
    MechanicDatabase& operator=(const MechanicDatabase&) = delete;

    struct SpellMechanicData
    {
        MechanicType type = MechanicType::NONE;
        float radius = 0.0f;
        float angle = 0.0f;
        bool requiresInterrupt = false;
        bool requiresDispel = false;
        bool requiresSoak = false;
    };

    ::std::unordered_map<uint32, SpellMechanicData> _spellMechanics;
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE> _mutex;
};

} // namespace Playerbot