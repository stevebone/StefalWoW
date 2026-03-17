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
#include "BotThreatManager.h"
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
class Spell;

namespace Playerbot
{

// Target selection priorities for different scenarios
enum class TargetPriority : uint8
{
    EMERGENCY = 0,      // Immediate attention required (low health ally under attack)
    CRITICAL = 1,       // High priority targets (healers, casters)
    INTERRUPT = 2,      // Targets casting important spells to interrupt
    PRIMARY = 3,        // Main target for sustained damage
    SECONDARY = 4,      // Alternative targets when primary unavailable
    CLEANUP = 5,        // Low priority cleanup targets
    IGNORE = 6          // Targets to ignore
};

// Target selection criteria
enum class SelectionCriteria : uint8
{
    THREAT = 0,         // Based on threat calculations
    HEALTH = 1,         // Target health percentage
    DISTANCE = 2,       // Proximity to bot
    ROLE = 3,           // Target's role (healer, tank, dps)
    VULNERABILITY = 4,  // Target's defensive state
    INTERRUPT = 5,      // Interrupt priority
    GROUP_FOCUS = 6,    // Group's current target
    CUSTOM = 7          // Custom selection logic
};

// Target validation flags
enum class TargetValidation : uint32
{
    NONE = 0x00000000,
    ALIVE = 0x00000001,
    IN_RANGE = 0x00000002,
    HOSTILE = 0x00000004,
    LINE_OF_SIGHT = 0x00000008,
    NOT_FRIENDLY = 0x00000010,
    NOT_IMMUNE = 0x00000020,
    NOT_EVADING = 0x00000040,
    NOT_CONFUSED = 0x00000080,
    IN_COMBAT = 0x00000100,
    SPELL_RANGE = 0x00000200,
    MELEE_RANGE_CHECK = 0x00000400,
    NOT_CROWD_CONTROLLED = 0x00000800,
    THREAT_REQUIRED = 0x00001000,

    // Common validation combinations
    BASIC = ALIVE | HOSTILE | NOT_FRIENDLY,
    COMBAT = BASIC | IN_RANGE | LINE_OF_SIGHT | NOT_IMMUNE,
    SPELL_TARGET = COMBAT | SPELL_RANGE | NOT_EVADING,
    MELEE_TARGET = COMBAT | MELEE_RANGE_CHECK | NOT_CONFUSED
};

// Enable bitwise operations for TargetValidation enum
inline TargetValidation operator|(TargetValidation a, TargetValidation b)
{
    return static_cast<TargetValidation>(static_cast<uint32>(a) | static_cast<uint32>(b));
}
inline TargetValidation operator&(TargetValidation a, TargetValidation b)
{
    return static_cast<TargetValidation>(static_cast<uint32>(a) & static_cast<uint32>(b));
}
inline bool HasFlag(TargetValidation a)
{
    return static_cast<uint32>(a) != 0;
}

// Target scoring weights for different selection criteria
struct TargetWeights
{
    float threatWeight = 1.0f;
    float healthWeight = 0.5f;
    float distanceWeight = 0.3f;
    float roleWeight = 0.8f;
    float vulnerabilityWeight = 0.6f;
    float interruptWeight = 1.5f;
    float groupFocusWeight = 0.7f;
    float customWeight = 0.0f;

    // Role-specific weight modifiers
    float healerPriority = 2.0f;
    float casterPriority = 1.5f;
    float tankPriority = 0.3f;
    float dpsPriority = 1.0f;
};

// Target information for selection
struct TargetInfo
{
    ObjectGuid guid;
    Unit* unit;
    float score;
    TargetPriority priority;
    float distance;
    float healthPercent;
    float threatLevel;
    bool isInterruptTarget;
    bool isGroupFocus;
    bool isVulnerable;
    uint32 lastUpdate;
    ::std::string reason;  // Why this target was selected

    TargetInfo() : unit(nullptr), score(0.0f), priority(TargetPriority::IGNORE),
                   distance(0.0f), healthPercent(100.0f), threatLevel(0.0f),
                   isInterruptTarget(false), isGroupFocus(false), isVulnerable(false),
                   lastUpdate(0) {}

    bool operator>(const TargetInfo& other) const
    {
        if (priority != other.priority)
            return priority < other.priority;  // Lower enum value = higher priority
        return score > other.score;
    }
};

// Target selection context for decision making
struct SelectionContext
{
    Player* bot;
    ThreatRole botRole;
    ::std::vector<Player*> groupMembers;
    Unit* currentTarget;
    Unit* groupTarget;
    uint32 spellId;
    float maxRange;
    bool inCombat;
    bool emergencyMode;
    TargetValidation validationFlags;
    TargetWeights weights;
    ::std::string selectionReason;

    SelectionContext() : bot(nullptr), botRole(ThreatRole::UNDEFINED), currentTarget(nullptr),
                        groupTarget(nullptr), spellId(0), maxRange(0.0f), inCombat(false),
                        emergencyMode(false), validationFlags(TargetValidation::BASIC) {}
};

// Target selection result
struct SelectionResult
{
    Unit* target;
    TargetInfo info;
    bool success;
    ::std::string failureReason;
    uint32 candidatesEvaluated;
    ::std::chrono::microseconds selectionTime;
    ::std::vector<TargetInfo> alternativeTargets;  // For backup selection

    SelectionResult() : target(nullptr), success(false), candidatesEvaluated(0),
                       selectionTime(::std::chrono::microseconds{0}) {}
};

// Performance metrics for target selection
struct SelectionMetrics
{
    ::std::atomic<uint32> totalSelections{0};
    ::std::atomic<uint32> successfulSelections{0};
    ::std::atomic<uint32> failedSelections{0};
    ::std::atomic<uint32> targetsEvaluated{0};
    ::std::chrono::microseconds averageSelectionTime{0};
    ::std::chrono::microseconds maxSelectionTime{0};
    ::std::chrono::steady_clock::time_point lastUpdate;

    void Reset()
    {
        totalSelections = 0;
        successfulSelections = 0;
        failedSelections = 0;
        targetsEvaluated = 0;
        averageSelectionTime = ::std::chrono::microseconds{0};
        maxSelectionTime = ::std::chrono::microseconds{0};
        lastUpdate = ::std::chrono::steady_clock::now();
    }
};

class TC_GAME_API TargetSelector
{
public:
    explicit TargetSelector(Player* bot, BotThreatManager* threatManager);
    ~TargetSelector() = default;

    // Primary target selection methods
    SelectionResult SelectBestTarget(const SelectionContext& context);
    SelectionResult SelectAttackTarget(float maxRange = 0.0f);
    SelectionResult SelectSpellTarget(uint32 spellId, float maxRange = 0.0f);
    SelectionResult SelectHealTarget(bool emergencyOnly = false);
    SelectionResult SelectInterruptTarget(float maxRange = 0.0f);
    SelectionResult SelectTankTarget();

    // Advanced selection methods
    SelectionResult SelectTargetByCriteria(SelectionCriteria criteria, const SelectionContext& context);
    SelectionResult SelectNearestTarget(float maxRange, TargetValidation validation = TargetValidation::BASIC);
    SelectionResult SelectWeakestTarget(float maxRange, TargetValidation validation = TargetValidation::BASIC);
    SelectionResult SelectStrongestTarget(float maxRange, TargetValidation validation = TargetValidation::BASIC);

    // Group coordination
    SelectionResult SelectGroupFocusTarget();
    void SetGroupTarget(Unit* target) { _groupTarget = target; }
    Unit* GetGroupTarget() const { return _groupTarget; }
    bool ShouldSwitchTarget(Unit* currentTarget, Unit* newTarget);

    // Target validation
    bool IsValidTarget(Unit* target, TargetValidation validation) const;
    bool IsInRange(Unit* target, float range) const;
    bool HasLineOfSight(Unit* target) const;
    bool CanAttack(Unit* target) const;
    bool CanCast(Unit* target, uint32 spellId) const;

    // Target scoring and evaluation
    float CalculateTargetScore(Unit* target, const SelectionContext& context);
    TargetPriority DetermineTargetPriority(Unit* target, const SelectionContext& context);
    ::std::vector<TargetInfo> EvaluateAllTargets(const SelectionContext& context);

    // Configuration
    void SetWeights(const TargetWeights& weights) { _weights = weights; }
    TargetWeights const& GetWeights() const { return _weights; }
    void SetMaxTargets(uint32 maxTargets) { _maxTargetsToEvaluate = maxTargets; }
    uint32 GetMaxTargets() const { return _maxTargetsToEvaluate; }

    // Performance monitoring
    SelectionMetrics const& GetMetrics() const { return _metrics; }
    void ResetMetrics() { _metrics.Reset(); }

    // Advanced features
    void PredictTargetMovement(Unit* target, uint32 timeAheadMs, Position& predictedPos);
    float EstimateTimeToKill(Unit* target);
    bool IsTargetFleeing(Unit* target);
    Unit* FindBestAoETarget(uint32 spellId, uint32 minTargets = 3);

    // Role-specific selection helpers
    Unit* SelectTankingTarget();
    Unit* SelectHealingTarget(bool criticalOnly = false);
    Unit* SelectDpsTarget();
    Unit* SelectSupportTarget();

    // Interrupt and crowd control
    Unit* SelectInterruptibleTarget(float maxRange = 0.0f);
    Unit* SelectCrowdControlTarget(uint32 spellId);
    Unit* SelectDispelTarget(bool hostileDispel = false);

    // Emergency response
    void EnableEmergencyMode() { _emergencyMode = true; }
    void DisableEmergencyMode() { _emergencyMode = false; }
    bool IsEmergencyMode() const { return _emergencyMode; }
    Unit* SelectEmergencyTarget();

private:
    // Internal selection methods
    ::std::vector<Unit*> GetNearbyEnemies(float range) const;
    ::std::vector<Unit*> GetNearbyAllies(float range) const;
    ::std::vector<Unit*> GetAllTargetCandidates(const SelectionContext& context) const;

    // Scoring components
    float CalculateThreatScore(Unit* target, const SelectionContext& context);
    float CalculateHealthScore(Unit* target, const SelectionContext& context);
    float CalculateDistanceScore(Unit* target, const SelectionContext& context);
    float CalculateRoleScore(Unit* target, const SelectionContext& context);
    float CalculateVulnerabilityScore(Unit* target, const SelectionContext& context);
    float CalculateInterruptScore(Unit* target, const SelectionContext& context);
    float CalculateGroupFocusScore(Unit* target, const SelectionContext& context);

    // Utility methods
    bool PassesValidation(Unit* target, TargetValidation validation) const;
    void UpdateMetrics(const SelectionResult& result);
    void LogSelection(const SelectionResult& result, const SelectionContext& context);

    // Target analysis
    bool IsHealer(Unit* target) const;
    bool IsCaster(Unit* target) const;
    bool IsTank(Unit* target) const;
    bool IsVulnerable(Unit* target) const;
    bool IsInterruptible(Unit* target) const;
    bool IsFleeing(Unit* target) const;

private:
    Player* _bot;
    BotThreatManager* _threatManager;
    Unit* _groupTarget;
    TargetWeights _weights;
    bool _emergencyMode;

    // Configuration
    uint32 _maxTargetsToEvaluate;
    uint32 _selectionCacheDuration;
    float _defaultMaxRange;

    // QW-2 FIX: Enhanced cache for performance - eliminates O(n²) redundant calculations
    // Threat score cache: Avoids double GetThreat() calls per candidate
    mutable ::std::unordered_map<ObjectGuid, float> _threatScoreCache;
    mutable uint32 _threatScoreCacheTimestamp;

    // Group focus cache: Pre-computed target counts from group members (O(m) once instead of O(n*m))
    mutable ::std::unordered_map<ObjectGuid, uint32> _groupFocusCache;
    mutable uint32 _groupFocusCacheTimestamp;

    // Legacy target cache (kept for compatibility)
    mutable ::std::unordered_map<ObjectGuid, TargetInfo> _targetCache;
    mutable uint32 _cacheTimestamp;
    mutable bool _cacheDirty;

    // Performance metrics
    mutable SelectionMetrics _metrics;

    // Thread safety
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE> _mutex;

    // Constants
    static constexpr uint32 DEFAULT_MAX_TARGETS = 50;
    static constexpr uint32 CACHE_DURATION_MS = 100;
    static constexpr float DEFAULT_MAX_RANGE = 40.0f;
    static constexpr float SELECTION_TIMEOUT_MS = 5.0f;

    // QW-2 FIX: Cache refresh intervals (500ms as specified in action plan)
    static constexpr uint32 THREAT_SCORE_CACHE_DURATION_MS = 500;
    static constexpr uint32 GROUP_FOCUS_CACHE_DURATION_MS = 500;

    // QW-2 FIX: Private helper methods for cache management
    void RefreshThreatScoreCache() const;
    void RefreshGroupFocusCache(const SelectionContext& context) const;
    float GetCachedThreatScore(Unit* target) const;
    uint32 GetCachedGroupFocusCount(Unit* target) const;

    // ST-3 FIX: Reusable vector buffers to eliminate per-call heap allocations
    // Research shows 250k vector allocations/sec for target selection in 5000-bot scenarios
    // These buffers are cleared and reused instead of allocating new vectors
    // Thread-safe: Class mutex is held during all selection operations
    mutable ::std::vector<Unit*> _enemiesBuffer;
    mutable ::std::vector<Unit*> _alliesBuffer;
    mutable ::std::vector<Unit*> _candidatesBuffer;
    mutable ::std::vector<TargetInfo> _evaluatedTargetsBuffer;

    // ST-3 FIX: Internal helpers that populate buffers (no allocation)
    void PopulateNearbyEnemies(float range) const;
    void PopulateNearbyAllies(float range) const;
};

// Target selection utilities
class TC_GAME_API TargetSelectionUtils
{
public:
    // Convenience methods for common selections
    static Unit* GetNearestEnemy(Player* bot, float maxRange = 40.0f);
    static Unit* GetWeakestEnemy(Player* bot, float maxRange = 40.0f);
    static Unit* GetStrongestEnemy(Player* bot, float maxRange = 40.0f);
    static Unit* GetMostWoundedAlly(Player* bot, float maxRange = 40.0f);
    static Unit* GetInterruptTarget(Player* bot, float maxRange = 30.0f);

    // Target analysis utilities
    static bool IsGoodHealTarget(Unit* target, Player* healer);
    static bool IsGoodInterruptTarget(Unit* target, Player* interrupter);
    static bool IsGoodTankTarget(Unit* target, Player* tank);
    static bool IsGoodDpsTarget(Unit* target, Player* dps);

    // Range and positioning utilities
    static float GetOptimalRange(Player* bot, Unit* target);
    static bool IsInOptimalPosition(Player* bot, Unit* target);
    static Position GetOptimalPosition(Player* bot, Unit* target);

    // Group coordination utilities
    static Unit* GetGroupConsensusTarget(const ::std::vector<Player*>& group);
    static bool ShouldFocusTarget(Unit* target, const ::std::vector<Player*>& group);
    static float CalculateGroupThreat(Unit* target, const ::std::vector<Player*>& group);
};

} // namespace Playerbot