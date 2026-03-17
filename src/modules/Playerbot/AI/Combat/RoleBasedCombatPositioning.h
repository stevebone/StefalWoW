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
#include "BotThreatManager.h"
#include "PositionManager.h"
#include "FormationManager.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <atomic>
#include <shared_mutex>
#include <chrono>
#include <optional>

// Forward declarations
class Player;
class Unit;
class Group;
class Spell;

namespace Playerbot
{

// Combat positioning strategies for different roles
enum class CombatPositionStrategy : uint8
{
    TANK_FRONTAL = 0,           // Tank faces boss away from group
    TANK_ROTATE = 1,            // Tank rotates boss for positional requirements
    HEALER_CENTRAL = 2,         // Healer maintains central position for max coverage
    HEALER_SAFE = 3,            // Healer prioritizes safety over coverage
    MELEE_BEHIND = 4,           // Melee DPS positions behind target
    MELEE_FLANK = 5,            // Melee DPS positions at flanks
    RANGED_SPREAD = 6,          // Ranged DPS spreads to minimize chain effects
    RANGED_STACK = 7,           // Ranged DPS stacks for AOE healing
    SUPPORT_FLEXIBLE = 8,       // Support adapts position based on needs
    EMERGENCY_SCATTER = 9       // Emergency spread for raid-wide mechanics
};

// Combat position requirements based on mechanics
enum class PositionalRequirement : uint32
{
    NONE = 0x00000000,
    BEHIND_TARGET = 0x00000001,     // Must be behind target (rogue backstab)
    FRONT_OF_TARGET = 0x00000002,   // Must be in front (tank)
    FLANK_TARGET = 0x00000004,      // Must be at side (positional abilities)
    MAX_MELEE_RANGE = 0x00000008,   // Must be in melee range
    MIN_RANGED = 0x00000010,        // Must maintain minimum range
    SPREAD_REQUIRED = 0x00000020,   // Must spread from others (chain mechanics)
    STACK_REQUIRED = 0x00000040,    // Must stack with group (shared damage)
    AVOID_FRONTAL = 0x00000080,     // Must avoid frontal cone
    AVOID_TAIL = 0x00000100,        // Must avoid tail sweep
    LOS_REQUIRED = 0x00000200,      // Must maintain line of sight
    SAFE_SPOT = 0x00000400,         // Must be in safe zone
    MOBILE_READY = 0x00000800,      // Must be ready to move quickly
    TANK_SWAP = 0x00001000,         // Positioning for tank swap mechanic
    INTERRUPT_RANGE = 0x00002000,   // Must be in interrupt range
    DISPEL_RANGE = 0x00004000,      // Must be in dispel range

    // Common role combinations
    TANK_REQUIREMENTS = FRONT_OF_TARGET | MAX_MELEE_RANGE | LOS_REQUIRED,
    MELEE_DPS_REQUIREMENTS = BEHIND_TARGET | MAX_MELEE_RANGE | AVOID_FRONTAL,
    RANGED_DPS_REQUIREMENTS = MIN_RANGED | SPREAD_REQUIRED | LOS_REQUIRED,
    HEALER_REQUIREMENTS = MIN_RANGED | LOS_REQUIRED | SAFE_SPOT | MOBILE_READY
};

DEFINE_ENUM_FLAG(PositionalRequirement);

// Tank positioning configuration
struct TankPositionConfig
{
    float optimalDistance = 3.0f;          // Optimal distance from boss
    float maxDistance = 5.0f;              // Maximum allowed distance
    float rotationSpeed = 2.0f;            // Boss rotation speed (rad/sec)
    float threatAngle = 180.0f;            // Angle to maintain from group
    bool autoFaceAway = true;              // Automatically face boss away
    bool maintainPosition = true;          // Try to minimize movement
    bool handleCleave = true;              // Position to avoid cleave hitting group
    float cleaveAngle = 90.0f;            // Frontal cleave cone angle
    float swapDistance = 8.0f;            // Distance for tank swap positioning
    uint32 positionCheckInterval = 500;    // Check position every X ms
};

// Healer positioning configuration
struct HealerPositionConfig
{
    float optimalRange = 25.0f;           // Optimal healing range
    float maxRange = 35.0f;                // Maximum healing range
    float minSafeDistance = 15.0f;        // Minimum distance from threats
    float groupCoverageRadius = 30.0f;    // Radius to cover group members
    bool prioritizeTankLOS = true;        // Always maintain tank LOS
    bool stayMobile = true;               // Maintain mobility for mechanics
    bool avoidMelee = true;               // Stay out of melee range
    float losCheckInterval = 250;         // LOS check frequency
    float safeSpotWeight = 2.0f;         // Weight for safe positioning
    uint32 coverageCheckInterval = 1000;  // Coverage check every X ms
};

// DPS positioning configuration
struct DPSPositionConfig
{
    // Melee DPS
    float meleeOptimalDistance = 3.0f;    // Optimal melee distance
    float meleeMaxDistance = 5.0f;        // Maximum melee distance
    bool preferBehind = true;             // Prefer behind positioning
    bool allowFlanking = true;            // Allow flank positioning
    float flankAngle = 90.0f;            // Angle for flanking
    float backstabAngle = 135.0f;        // Required angle for backstab

    // Ranged DPS
    float rangedOptimalDistance = 25.0f;  // Optimal ranged distance
    float rangedMinDistance = 8.0f;       // Minimum safe distance
    float rangedMaxDistance = 35.0f;      // Maximum casting range
    float spreadDistance = 8.0f;          // Spread distance from others
    bool maintainSpread = true;           // Maintain spread positioning
    bool allowStacking = false;           // Allow stacking when needed

    uint32 positionUpdateInterval = 750;  // Position update frequency
};

// Position scoring for role-based evaluation
struct RolePositionScore
{
    Position position;
    float totalScore = 0.0f;
    float roleScore = 0.0f;           // Role-specific score
    float mechanicScore = 0.0f;       // Mechanic avoidance score
    float safetyScore = 0.0f;         // General safety score
    float efficiencyScore = 0.0f;     // DPS/healing efficiency score
    float mobilityScore = 0.0f;       // Ability to respond to mechanics
    PositionalRequirement metRequirements = PositionalRequirement::NONE;
    PositionalRequirement failedRequirements = PositionalRequirement::NONE;
    ::std::string reasoning;
    bool isValid = false;

    bool operator>(const RolePositionScore& other) const
    {
        return totalScore > other.totalScore;
    }
};

// Combat positioning context
struct CombatPositionContext
{
    Player* bot = nullptr;
    Unit* primaryTarget = nullptr;
    Unit* currentThreat = nullptr;
    Group* group = nullptr;
    ThreatRole role = ThreatRole::UNDEFINED;
    CombatPositionStrategy strategy = CombatPositionStrategy::SUPPORT_FLEXIBLE;
    PositionalRequirement requirements = PositionalRequirement::NONE;

    // Group information
    ::std::vector<Player*> tanks;
    ::std::vector<Player*> healers;
    ::std::vector<Player*> meleeDPS;
    ::std::vector<Player*> rangedDPS;
    Player* mainTank = nullptr;
    Player* offTank = nullptr;

    // Combat state
    bool inCombat = false;
    bool isTankSwap = false;
    bool hasIncomingDamage = false;
    bool requiresMovement = false;
    uint32 combatTime = 0;

    // Mechanic information
    ::std::vector<Position> dangerZones;
    ::std::vector<Position> safeZones;
    float cleaveAngle = 0.0f;
    float tailSwipeAngle = 0.0f;
    bool hasActiveAOE = false;
    bool requiresSpread = false;
    bool requiresStack = false;

    // Performance constraints
    uint32 maxCalculationTime = 50;    // Max 50ms for position calculation
    uint32 maxCandidates = 24;         // Maximum position candidates
};

// Tank-specific positioning logic
class TC_GAME_API TankPositioning
{
public:
    explicit TankPositioning(const TankPositionConfig& config = {});
    ~TankPositioning() = default;

    // Main tank positioning
    Position CalculateTankPosition(Unit* target, Group* group, const CombatPositionContext& context);
    Position CalculateOffTankPosition(Unit* target, Player* mainTank, const CombatPositionContext& context);

    // Threat and facing management
    void HandleThreatPositioning(Player* tank, Unit* target);
    float CalculateOptimalFacing(Unit* target, const ::std::vector<Player*>& groupMembers);
    bool ShouldRotateBoss(Unit* target, float currentFacing, float desiredFacing);

    // Cleave and mechanic handling
    void ManageCleaveMechanics(Unit* target, const ::std::vector<Player*>& groupMembers);
    Position CalculateCleaveAvoidancePosition(Unit* target, float cleaveAngle);
    bool IsGroupSafeFromCleave(Unit* target, const Position& tankPos, float cleaveAngle);

    // Tank swap mechanics
    Position CalculateTankSwapPosition(Player* currentTank, Player* swapTank, Unit* target);
    bool IsInSwapPosition(Player* tank, Player* otherTank, Unit* target);
    float GetSwapDistance() const { return _config.swapDistance; }

    // Position optimization
    Position FindOptimalTankSpot(Unit* target, float minDistance = 3.0f, float maxDistance = 5.0f);
    ::std::vector<RolePositionScore> EvaluateTankPositions(const ::std::vector<Position>& candidates,
                                                          const CombatPositionContext& context);

    // Configuration
    void SetConfig(const TankPositionConfig& config) { _config = config; }
    const TankPositionConfig& GetConfig() const { return _config; }

private:
    Position CalculateFrontalPosition(Unit* target, float distance);
    float CalculateThreatAngle(const Position& tankPos, const Position& targetPos,
                               const ::std::vector<Player*>& group);
    bool ValidateTankPosition(const Position& pos, Unit* target, const CombatPositionContext& context);
    float ScoreTankPosition(const Position& pos, const CombatPositionContext& context);

private:
    TankPositionConfig _config;
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE> _mutex;

    // Constants
    static constexpr float MIN_TANK_DISTANCE = 2.0f;
    static constexpr float MAX_TANK_DISTANCE = 7.0f;
    static constexpr float IDEAL_THREAT_ANGLE = 180.0f;
    static constexpr float CLEAVE_SAFETY_MARGIN = 10.0f;
};

// Healer-specific positioning logic
class TC_GAME_API HealerPositioning
{
public:
    explicit HealerPositioning(const HealerPositionConfig& config = {});
    ~HealerPositioning() = default;

    // Main healer positioning
    Position CalculateHealerPosition(Group* group, Unit* combatTarget, const CombatPositionContext& context);
    Position CalculateRaidHealerPosition(const ::std::vector<Player*>& raidMembers, const CombatPositionContext& context);
    Position CalculateTankHealerPosition(Player* tank, Unit* threat, const CombatPositionContext& context);

    // Range and coverage optimization
    bool IsInOptimalHealingRange(Player* healer, const ::std::vector<Player*>& allies);
    float CalculateHealingCoverage(const Position& healerPos, const ::std::vector<Player*>& allies);
    Position OptimizeHealingCoverage(Player* healer, const ::std::vector<Player*>& allies);

    // Safety and positioning
    Position FindSafeHealingSpot(Player* healer, Unit* threat, const CombatPositionContext& context);
    bool IsPositionSafeForHealing(const Position& pos, const CombatPositionContext& context);
    float CalculateSafetyScore(const Position& pos, const ::std::vector<Unit*>& threats);

    // Line of sight management
    void MaintainLineOfSight(Player* healer, const ::std::vector<Player*>& allies);
    bool HasLineOfSightToAll(const Position& healerPos, const ::std::vector<Player*>& allies);
    Position FindBestLOSPosition(Player* healer, const ::std::vector<Player*>& priorityTargets);

    // Multi-healer coordination
    ::std::vector<Position> CalculateMultiHealerPositions(const ::std::vector<Player*>& healers,
                                                         const ::std::vector<Player*>& group);
    void CoordinateHealerPositioning(const ::std::vector<Player*>& healers, Group* group);

    // Position evaluation
    ::std::vector<RolePositionScore> EvaluateHealerPositions(const ::std::vector<Position>& candidates,
                                                            const CombatPositionContext& context);

    // Configuration
    void SetConfig(const HealerPositionConfig& config) { _config = config; }
    const HealerPositionConfig& GetConfig() const { return _config; }

private:
    float CalculateHealerScore(const Position& pos, const CombatPositionContext& context);
    bool ValidateHealerPosition(const Position& pos, const CombatPositionContext& context);
    Position CalculateCenterOfCare(const ::std::vector<Player*>& allies);
    float GetEffectiveHealingRange(Player* healer);

private:
    HealerPositionConfig _config;
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE> _mutex;

    // Constants
    static constexpr float MIN_HEALER_DISTANCE = 15.0f;
    static constexpr float MAX_HEALER_DISTANCE = 40.0f;
    static constexpr float OPTIMAL_COVERAGE_RADIUS = 30.0f;
    static constexpr float LOS_CHECK_HEIGHT = 2.0f;
};

// DPS-specific positioning logic
class TC_GAME_API DPSPositioning
{
public:
    explicit DPSPositioning(const DPSPositionConfig& config = {});
    ~DPSPositioning() = default;

    // Melee DPS positioning
    Position CalculateMeleeDPSPosition(Unit* target, Player* tank, const CombatPositionContext& context);
    Position CalculateBackstabPosition(Unit* target, float requiredAngle = 135.0f);
    Position CalculateFlankPosition(Unit* target, bool leftFlank = true);
    void DistributeMeleePositions(const ::std::vector<Player*>& meleeDPS, Unit* target, Player* tank);

    // Ranged DPS positioning
    Position CalculateRangedDPSPosition(Unit* target, float optimalRange, const CombatPositionContext& context);
    void SpreadRangedPositions(const ::std::vector<Player*>& rangedDPS, Unit* target, float spreadDistance);
    Position CalculateCasterPosition(Player* caster, Unit* target, uint32 spellId);

    // Cleave and mechanic avoidance
    void AvoidFrontalCleaves(Player* dps, Unit* target, float cleaveAngle);
    void AvoidTailSwipe(Player* dps, Unit* target, float swipeAngle);
    bool IsInCleaveDanger(const Position& pos, Unit* target, float cleaveAngle);

    // Positional requirements
    void HandlePositionalRequirements(Player* dps, uint32 spellId);
    bool MeetsPositionalRequirement(Player* dps, Unit* target, PositionalRequirement req);
    Position FindPositionForRequirement(Unit* target, PositionalRequirement req);

    // DPS optimization
    Position OptimizeDPSPosition(Player* dps, Unit* target, const CombatPositionContext& context);
    float CalculateDPSEfficiency(const Position& pos, Player* dps, Unit* target);

    // Multi-target positioning
    Position CalculateAOEPosition(Player* dps, const ::std::vector<Unit*>& targets);
    Position CalculateCleaveDPSPosition(Player* dps, const ::std::vector<Unit*>& targets);

    // Position evaluation
    ::std::vector<RolePositionScore> EvaluateDPSPositions(const ::std::vector<Position>& candidates,
                                                         Player* dps, const CombatPositionContext& context);

    // Configuration
    void SetConfig(const DPSPositionConfig& config) { _config = config; }
    const DPSPositionConfig& GetConfig() const { return _config; }

private:
    float CalculateDPSScore(const Position& pos, Player* dps, const CombatPositionContext& context);
    bool ValidateDPSPosition(const Position& pos, Player* dps, const CombatPositionContext& context);
    float GetOptimalDPSRange(Player* dps, Unit* target);
    Position RotateAroundTarget(Unit* target, float angle, float distance);

private:
    DPSPositionConfig _config;
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE> _mutex;

    // Constants
    static constexpr float MELEE_MIN_DISTANCE = 2.0f;
    static constexpr float MELEE_MAX_DISTANCE = 5.0f;
    static constexpr float RANGED_MIN_DISTANCE = 8.0f;
    static constexpr float RANGED_MAX_DISTANCE = 40.0f;
    static constexpr float BACKSTAB_ANGLE_TOLERANCE = 45.0f;
};

// Main role-based combat positioning system
class TC_GAME_API RoleBasedCombatPositioning
{
public:
    RoleBasedCombatPositioning();
    ~RoleBasedCombatPositioning() = default;

    // Initialize with existing systems
    void Initialize(PositionManager* positionMgr, BotThreatManager* threatMgr, FormationManager* formationMgr);

    // Main positioning interface
    Position CalculateCombatPosition(Player* bot, const CombatPositionContext& context);
    PositionMovementResult UpdateCombatPosition(Player* bot, const CombatPositionContext& context);

    // Role-specific position calculation
    Position CalculateRolePosition(Player* bot, ThreatRole role, const CombatPositionContext& context);
    Position CalculateTankPosition(Player* tank, const CombatPositionContext& context);
    Position CalculateHealerPosition(Player* healer, const CombatPositionContext& context);
    Position CalculateDPSPosition(Player* dps, const CombatPositionContext& context);

    // Strategy selection
    CombatPositionStrategy SelectStrategy(const CombatPositionContext& context);
    void UpdateStrategy(Player* bot, CombatPositionStrategy newStrategy);

    // Positional requirements
    PositionalRequirement GetPositionalRequirements(Player* bot, Unit* target);
    bool ValidatePositionalRequirements(const Position& pos, PositionalRequirement requirements,
                                       const CombatPositionContext& context);

    // Group coordination
    void CoordinateGroupPositioning(Group* group, Unit* target);
    void OptimizeRoleDistribution(Group* group);
    ::std::unordered_map<ObjectGuid, Position> CalculateGroupFormation(Group* group, Unit* target);

    // Dynamic adjustment
    void AdjustForMechanics(Player* bot, const ::std::vector<Position>& dangerZones);
    void RespondToEmergency(Player* bot, const Position& safeZone);

    // Performance monitoring
    float GetAverageCalculationTime() const;
    uint32 GetPositionUpdateCount() const;
    void ResetPerformanceMetrics();

    // Configuration management
    void SetTankConfig(const TankPositionConfig& config);
    void SetHealerConfig(const HealerPositionConfig& config);
    void SetDPSConfig(const DPSPositionConfig& config);

    // Integration with existing systems
    void SetPositionManager(PositionManager* mgr) { _positionManager = mgr; }
    void SetThreatManager(BotThreatManager* mgr) { _threatManager = mgr; }
    void SetFormationManager(FormationManager* mgr) { _formationManager = mgr; }

    // Static utility functions
    static ThreatRole DetermineRole(Player* bot);
    static bool IsRoleCompatible(ThreatRole role, CombatPositionStrategy strategy);
    static float CalculateRoleEfficiency(Player* bot, ThreatRole role, const Position& pos);

private:
    // Position calculation helpers
    Position CalculatePositionByStrategy(Player* bot, CombatPositionStrategy strategy,
                                        const CombatPositionContext& context);
    ::std::vector<Position> GenerateCandidatePositions(Player* bot, const CombatPositionContext& context);
    RolePositionScore EvaluatePosition(const Position& pos, Player* bot,
                                       const CombatPositionContext& context);

    // Context analysis
    CombatPositionContext AnalyzeCombatContext(Player* bot, Group* group);
    void UpdateGroupRoles(Group* group, CombatPositionContext& context);
    void IdentifyDangerZones(Unit* target, CombatPositionContext& context);

    // Performance tracking
    void TrackCalculationTime(::std::chrono::microseconds duration);

private:
    // Component systems
    ::std::unique_ptr<TankPositioning> _tankPositioning;
    ::std::unique_ptr<HealerPositioning> _healerPositioning;
    ::std::unique_ptr<DPSPositioning> _dpsPositioning;

    // Integration with existing systems
    PositionManager* _positionManager;
    BotThreatManager* _threatManager;
    FormationManager* _formationManager;

    // Strategy cache
    ::std::unordered_map<ObjectGuid, CombatPositionStrategy> _strategyCache;
    ::std::unordered_map<ObjectGuid, uint32> _lastStrategyUpdate;

    // Performance metrics
    ::std::atomic<uint32> _positionUpdates{0};
    ::std::atomic<uint32> _calculationCount{0};
    ::std::chrono::microseconds _totalCalculationTime{0};
    ::std::chrono::microseconds _averageCalculationTime{0};

    // Thread safety
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE> _mutex;

    // Constants
    static constexpr uint32 STRATEGY_UPDATE_INTERVAL = 5000;  // 5 seconds
    static constexpr uint32 MAX_CALCULATION_TIME = 100;       // 100ms max
    static constexpr float POSITION_UPDATE_THRESHOLD = 3.0f;  // 3 yards
};

} // namespace Playerbot