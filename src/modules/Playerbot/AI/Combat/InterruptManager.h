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
#include "SharedDefines.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <atomic>
#include <shared_mutex>
#include <chrono>

// Forward declarations
class Player;
class Unit;
class Spell;
class SpellInfo;

namespace Playerbot
{

// Interrupt priority levels for WoW 12.0 encounters
enum class InterruptPriority : uint8
{
    CRITICAL = 0,           // Must interrupt immediately (healing, fear, etc.)
    HIGH = 1,               // High priority interrupt (major damage, CC)
    MODERATE = 2,           // Standard interrupt priority
    LOW = 3,                // Low priority interrupt
    IGNORE = 4              // Do not interrupt
};

// Interrupt assignment states for group coordination
enum class InterruptAssignment : uint8
{
    UNASSIGNED = 0,         // No one assigned
    PRIMARY = 1,            // Primary interrupter
    BACKUP = 2,             // Backup interrupter
    TERTIARY = 3,           // Tertiary interrupter
    EXCLUDED = 4            // Excluded from interrupt rotation
};

// Interrupt types based on spell effects
enum class InterruptType : uint8
{
    DAMAGE_PREVENTION = 0,  // Prevent high damage spells
    HEALING_DENIAL = 1,     // Prevent healing spells
    CROWD_CONTROL = 2,      // Prevent CC effects
    BUFF_DENIAL = 3,        // Prevent beneficial buffs
    DEBUFF_PREVENTION = 4,  // Prevent harmful debuffs
    CHANNEL_BREAK = 5,      // Break channeled spells
    CAST_DELAY = 6,         // Delay important casts
    RESOURCE_DENIAL = 7     // Prevent resource generation
};

// Interrupt method types
enum class InterruptMethod : uint8
{
    SPELL_INTERRUPT = 0,    // Direct spell interrupt (Counterspell, etc.)
    STUN = 1,              // Stun to interrupt
    KNOCKBACK = 2,         // Knockback interrupt
    SILENCE = 3,           // Silence effect
    DISPEL = 4,            // Dispel the spell
    LINE_OF_SIGHT = 5,     // Break LoS to interrupt
    MOVEMENT = 6,          // Movement-based interrupt
    FEAR = 7               // Fear to interrupt
};

// Interrupt target information for WoW 12.0
struct InterruptTarget
{
    ObjectGuid guid;
    Unit* unit;
    Position position;
    Spell* currentSpell;
    const SpellInfo* spellInfo;
    uint32 spellId;
    InterruptPriority priority;
    InterruptType type;
    float remainingCastTime;
    float totalCastTime;
    float castProgress;
    uint32 detectedTime;
    uint32 timeWindow;
    bool isChanneled;
    bool isInterruptible;
    bool requiresLoS;
    ::std::string spellName;
    ::std::string targetName;

    // WoW 12.0 specific fields
    bool isMythicPlus;
    uint8 mythicLevel;
    bool hasInterruptImmunity;
    uint32 immunityExpires;
    float threatMultiplier;
    bool isKeyAbility;      // Key ability that must be interrupted
    uint32 schoolMask;      // Spell school for lockout tracking
    ObjectGuid assignedInterrupter;  // Who's assigned to interrupt
    InterruptAssignment assignmentStatus;

    InterruptTarget() : unit(nullptr), currentSpell(nullptr), spellInfo(nullptr),
                       spellId(0), priority(InterruptPriority::IGNORE),
                       type(InterruptType::DAMAGE_PREVENTION), remainingCastTime(0.0f),
                       totalCastTime(0.0f), castProgress(0.0f), detectedTime(0),
                       timeWindow(0), isChanneled(false), isInterruptible(false),
                       requiresLoS(true), isMythicPlus(false), mythicLevel(0),
                       hasInterruptImmunity(false), immunityExpires(0),
                       threatMultiplier(1.0f), isKeyAbility(false), schoolMask(0),
                       assignmentStatus(InterruptAssignment::UNASSIGNED) {}
};

// Interrupt capability information for WoW 12.0 abilities
struct InterruptCapability
{
    uint32 spellId;
    ::std::string spellName;
    InterruptMethod method;
    float range;
    float cooldown;
    uint32 manaCost;
    uint32 resourceCost;     // For non-mana resources
    Powers resourceType;     // Resource type (Energy, Rage, etc.)
    float castTime;
    bool requiresLoS;
    bool requiresFacing;
    bool isAvailable;
    uint32 lastUsed;
    uint32 cooldownExpires;
    InterruptPriority minPriority;
    ::std::vector<InterruptType> effectiveAgainst;

    // WoW 12.0 specific fields
    uint32 lockoutDuration;  // Duration of spell school lockout
    uint32 schoolMask;       // Which schools this locks out
    bool isHeroTalent;       // Hero talent ability
    bool requiresSpec;       // Requires specific specialization
    uint32 requiredSpec;     // Required spec ID
    float globalCooldown;    // GCD duration
    bool offGCD;            // Can be used off GCD
    uint8 charges;          // Number of charges (for abilities like Wind Shear)
    uint8 currentCharges;
    uint32 chargeRecoveryTime;

    InterruptCapability() : spellId(0), method(InterruptMethod::SPELL_INTERRUPT),
                           range(0.0f), cooldown(0.0f), manaCost(0), resourceCost(0),
                           resourceType(POWER_MANA), castTime(0.0f),
                           requiresLoS(true), requiresFacing(true), isAvailable(false),
                           lastUsed(0), cooldownExpires(0), minPriority(InterruptPriority::MODERATE),
                           lockoutDuration(0), schoolMask(0), isHeroTalent(false),
                           requiresSpec(false), requiredSpec(0), globalCooldown(1.5f),
                           offGCD(false), charges(1), currentCharges(1), chargeRecoveryTime(0) {}
};

// Interrupt execution plan
struct InterruptPlan
{
    InterruptTarget* target;
    InterruptCapability* capability;
    InterruptMethod method;
    float executionTime;
    float successProbability;
    float reactionTime;
    bool requiresMovement;
    Position executionPosition;
    uint32 priority;
    ::std::string reasoning;

    InterruptPlan() : target(nullptr), capability(nullptr), method(InterruptMethod::SPELL_INTERRUPT),
                     executionTime(0.0f), successProbability(0.0f), reactionTime(0.0f),
                     requiresMovement(false), priority(0) {}

    bool operator<(const InterruptPlan& other) const
    {
        if (priority != other.priority)
            return priority < other.priority;
        return successProbability > other.successProbability;
    }
};

// Interrupt execution result
struct InterruptResult
{
    bool success;
    bool attemptMade;
    InterruptMethod usedMethod;
    uint32 usedSpell;
    float timingAccuracy;
    uint32 executionTime;
    ::std::string failureReason;
    InterruptTarget originalTarget;

    InterruptResult() : success(false), attemptMade(false),
                       usedMethod(InterruptMethod::SPELL_INTERRUPT), usedSpell(0),
                       timingAccuracy(0.0f), executionTime(0) {}
};

// Interrupt performance metrics
struct InterruptMetrics
{
    ::std::atomic<uint32> interruptAttempts{0};
    ::std::atomic<uint32> successfulInterrupts{0};
    ::std::atomic<uint32> failedInterrupts{0};
    ::std::atomic<uint32> missedOpportunities{0};
    ::std::atomic<uint32> criticalInterrupts{0};
    ::std::chrono::microseconds averageReactionTime{0};
    ::std::chrono::microseconds minReactionTime{::std::chrono::microseconds::max()};
    ::std::chrono::microseconds maxReactionTime{0};
    float averageTimingAccuracy{0.0f};
    ::std::chrono::steady_clock::time_point lastUpdate;

    void Reset()
    {
        interruptAttempts = 0;
        successfulInterrupts = 0;
        failedInterrupts = 0;
        missedOpportunities = 0;
        criticalInterrupts = 0;
        averageReactionTime = ::std::chrono::microseconds{0};
        minReactionTime = ::std::chrono::microseconds::max();
        maxReactionTime = ::std::chrono::microseconds{0};
        averageTimingAccuracy = 0.0f;
        lastUpdate = ::std::chrono::steady_clock::now();
    }

    float GetSuccessRate() const
    {
        uint32 total = interruptAttempts.load();
        return total > 0 ? static_cast<float>(successfulInterrupts.load()) / total : 0.0f;
    }
};

// Group interrupt coordination data
struct GroupInterruptData
{
    ::std::unordered_map<ObjectGuid, InterruptAssignment> assignments;
    ::std::unordered_map<ObjectGuid, ::std::vector<InterruptCapability>> memberCapabilities;
    ::std::unordered_map<ObjectGuid, uint32> memberCooldowns;
    ::std::unordered_map<uint32, ObjectGuid> spellAssignments;  // SpellId -> Assigned player
    uint32 lastRotationUpdate;
    uint8 rotationIndex;
    ::std::vector<ObjectGuid> rotationOrder;

    GroupInterruptData() : lastRotationUpdate(0), rotationIndex(0) {}
};

// Interrupt rotation strategy
struct InterruptRotation
{
    ::std::vector<ObjectGuid> primaryInterrupters;
    ::std::vector<ObjectGuid> backupInterrupters;
    ::std::unordered_map<ObjectGuid, uint32> nextAvailable;  // When each member's interrupt is available
    uint32 rotationCooldown;  // Time between rotation switches
    bool useStaggered;        // Use staggered interrupts for chain casting

    InterruptRotation() : rotationCooldown(10000), useStaggered(false) {}
};

class TC_GAME_API InterruptManager
{
public:
    explicit InterruptManager(Player* bot);
    ~InterruptManager() = default;

    // Primary interrupt interface
    void UpdateInterruptSystem(uint32 diff);
    void Update(uint32 diff) { UpdateInterruptSystem(diff); }  // Alias for compatibility
    ::std::vector<InterruptTarget> ScanForInterruptTargets();
    InterruptResult AttemptInterrupt(const InterruptTarget& target);
    void ProcessInterruptOpportunities();

    // Compatibility interface for CombatBehaviorIntegration
    bool HasUrgentInterrupt() const;
    Unit* GetInterruptTarget();
    bool ShouldInterrupt(Unit* target);
    bool ShouldInterruptOwnCast();
    bool IsCastDangerous(Unit* caster);
    bool IsCastHighPriority(Unit* caster);
    void Reset();

    // Target analysis and prioritization
    InterruptPriority AssessInterruptPriority(const SpellInfo* spellInfo, Unit* caster);
    InterruptType ClassifyInterruptType(const SpellInfo* spellInfo);
    bool IsSpellInterruptWorthy(uint32 spellId, Unit* caster);
    float CalculateInterruptUrgency(const InterruptTarget& target);

    // Capability management
    void InitializeInterruptCapabilities();
    void UpdateInterruptCapabilities();
    ::std::vector<InterruptCapability> GetAvailableInterrupts();
    InterruptCapability* GetBestInterruptForTarget(const InterruptTarget& target);
    bool CanInterrupt(uint32 spellId, InterruptMethod method = InterruptMethod::SPELL_INTERRUPT);

    // Interrupt planning and execution
    InterruptPlan CreateInterruptPlan(const InterruptTarget& target);
    ::std::vector<InterruptPlan> GenerateInterruptPlans(const ::std::vector<InterruptTarget>& targets);
    bool ExecuteInterruptPlan(const InterruptPlan& plan);
    bool IsInterruptExecutable(const InterruptPlan& plan);

    // Spell-specific interrupt logic
    bool ShouldInterruptHealing(const SpellInfo* spellInfo, Unit* caster);
    bool ShouldInterruptCrowdControl(const SpellInfo* spellInfo, Unit* caster);
    bool ShouldInterruptDamage(const SpellInfo* spellInfo, Unit* caster);
    bool ShouldInterruptBuff(const SpellInfo* spellInfo, Unit* caster);

    // Timing and prediction
    float PredictCastCompletion(const InterruptTarget& target);
    float CalculateInterruptWindow(const InterruptTarget& target, InterruptMethod method);
    bool CanInterruptInTime(const InterruptTarget& target, InterruptMethod method);
    uint32 GetOptimalInterruptTiming(const InterruptTarget& target);

    // Group coordination for WoW 12.0
    void CoordinateInterruptsWithGroup(const ::std::vector<Player*>& groupMembers);
    bool ShouldLetOthersInterrupt(const InterruptTarget& target);
    void RegisterInterruptAttempt(const InterruptTarget& target);
    void ShareInterruptInformation(const ::std::vector<Player*>& groupMembers);

    // Interrupt rotation management
    void InitializeInterruptRotation(const ::std::vector<Player*>& groupMembers);
    void UpdateInterruptRotation();
    InterruptAssignment GetMyAssignment(const InterruptTarget& target);
    ObjectGuid GetAssignedInterrupter(const InterruptTarget& target);
    void AssignInterruptRoles(const ::std::vector<Player*>& groupMembers);
    void OptimizeRotationForEncounter(uint32 encounterId);

    // Cooldown tracking across group
    void TrackGroupCooldowns(const ::std::vector<Player*>& groupMembers);
    uint32 GetMemberInterruptCooldown(ObjectGuid memberGuid);
    bool IsMemberInterruptReady(ObjectGuid memberGuid);
    ::std::vector<ObjectGuid> GetAvailableInterrupters();

    // Backup interrupt systems
    bool TriggerBackupInterrupt(const InterruptTarget& target);
    void PromoteBackupToMain(const InterruptTarget& target);
    bool HandleInterruptFailure(const InterruptTarget& target, const InterruptResult& result);
    void RequestEmergencyAssist(const InterruptTarget& target);

    // Advanced interrupt strategies for WoW 12.0
    void HandleMultipleInterruptTargets();
    void OptimizeInterruptRotation();
    void ManageInterruptCooldowns();
    void HandleInterruptImmunity(Unit* target);
    void HandleMythicPlusAffixes(uint8 mythicLevel);
    void PredictChainCasting(Unit* caster);
    void HandleSchoolLockouts(uint32 schoolMask, uint32 duration);
    void AdaptToEncounterMechanics(uint32 encounterId);

    // Configuration and management
    void SetReactionTime(uint32 reactionTimeMs) { _reactionTime = reactionTimeMs; }
    uint32 GetReactionTime() const { return _reactionTime; }
    void SetInterruptRange(float range) { _maxInterruptRange = range; }
    float GetInterruptRange() const { return _maxInterruptRange; }
    void EnablePredictiveInterrupts(bool enable) { _predictiveInterrupts = enable; }
    bool IsPredictiveInterruptsEnabled() const { return _predictiveInterrupts; }

    // Performance monitoring
    InterruptMetrics const& GetMetrics() const { return _metrics; }
    void ResetMetrics() { _metrics.Reset(); }
    float GetInterruptUptime() const;
    float GetRotationEfficiency() const;
    void LogRotationPerformance();

    // Query methods
    bool IsCurrentlyInterrupting() const { return _isInterrupting; }
    InterruptTarget* GetCurrentInterruptTarget() { return _currentTarget; }
    uint32 GetInterruptCapabilityCount() const { return static_cast<uint32>(_interruptCapabilities.size()); }

    // Emergency interrupt handling
    void ActivateEmergencyInterruptMode() { _emergencyMode = true; }
    void DeactivateEmergencyInterruptMode() { _emergencyMode = false; }
    bool IsEmergencyMode() const { return _emergencyMode; }
    InterruptResult ExecuteEmergencyInterrupt(Unit* target);

private:
    // Core interrupt detection
    void ScanNearbyUnitsForCasts();
    bool IsValidInterruptTarget(Unit* unit);
    void AnalyzeCastingSpell(Unit* caster);
    void UpdateTargetInformation(InterruptTarget& target);

    // Priority assessment methods
    InterruptPriority AssessHealingPriority(const SpellInfo* spellInfo, Unit* caster);
    InterruptPriority AssessDamagePriority(const SpellInfo* spellInfo, Unit* caster);
    InterruptPriority AssessCrowdControlPriority(const SpellInfo* spellInfo, Unit* caster);
    InterruptPriority AssessBuffPriority(const SpellInfo* spellInfo, Unit* caster);

    // Capability analysis
    void LoadClassSpecificInterrupts();
    void CheckInterruptAvailability();
    void UpdateCooldownStatus();
    float CalculateInterruptEffectiveness(const InterruptCapability& capability, const InterruptTarget& target);

    // Execution methods
    bool CastInterruptSpell(uint32 spellId, Unit* target);
    bool AttemptLoSInterrupt(Unit* target);
    bool AttemptMovementInterrupt(Unit* target);
    void HandleInterruptFailure(const InterruptPlan& plan);
    bool FindCoverPosition(Unit* target, Position& outPos);

    // Timing calculations
    float CalculateReactionDelay();
    float CalculateExecutionTime(InterruptMethod method);
    float CalculateCastTimeRemaining(Unit* caster);
    bool IsWithinInterruptWindow(const InterruptTarget& target, float executionTime);

    // Group coordination helpers
    bool IsGroupMemberInterrupting(const InterruptTarget& target);
    void BroadcastInterruptIntent(const InterruptTarget& target);
    void ReceiveInterruptCoordination(Player* sender, const InterruptTarget& target);

    // Performance tracking
    void TrackInterruptAttempt(const InterruptResult& result);
    void UpdateReactionTimeMetrics(::std::chrono::microseconds reactionTime);
    void UpdateTimingAccuracy(const InterruptResult& result);

    // Utility methods
    bool HasLineOfSightToTarget(Unit* target);
    bool IsInRangeForInterrupt(Unit* target, float range);
    Position CalculateOptimalInterruptPosition(Unit* target);

private:
    Player* _bot;

    // Current state
    bool _isInterrupting;
    InterruptTarget* _currentTarget;
    ::std::vector<InterruptTarget> _trackedTargets;
    ::std::vector<InterruptCapability> _interruptCapabilities;

    // WoW 12.0 specific tracking
    ::std::unordered_map<uint32, uint32> _schoolLockouts;  // School mask -> expiry time
    ::std::unordered_map<ObjectGuid, uint32> _targetImmunities;  // Target -> immunity expiry
    GroupInterruptData _groupData;
    InterruptRotation _currentRotation;
    ::std::unordered_map<uint32, InterruptPriority> _spellPriorityOverrides;  // Custom priorities

    // Configuration
    uint32 _reactionTime;
    float _maxInterruptRange;
    uint32 _scanInterval;
    bool _predictiveInterrupts;
    bool _emergencyMode;
    float _timingAccuracyTarget;

    // Timing tracking
    uint32 _lastScan;
    uint32 _lastInterruptAttempt;
    ::std::unordered_map<ObjectGuid, uint32> _targetFirstDetected;

    // Group coordination
    ::std::unordered_map<ObjectGuid, uint32> _groupInterruptClaims;
    uint32 _lastCoordinationUpdate;

    // Deferred initialization flag
    // CRITICAL: Capabilities cannot be initialized in constructor because bot's
    // power systems (GetPower()) are not ready yet, causing ACCESS_VIOLATION
    // in CalcPowerCost(). Initialize on first Update() when bot is fully ready.
    bool _capabilitiesInitialized;

    // Performance metrics
    mutable InterruptMetrics _metrics;

    // Thread safety
    // DEADLOCK FIX #15: Changed from std::shared_mutex to std::recursive_mutex
    // The root cause of persistent deadlock was InterruptManager._mutex, NOT BotAI._mutex.
    // ScanForInterruptTargets() is called from multiple threads:
    //   - Thread 1: ClassAI::UpdateRotation() -> MageAI::GetBestCounterspellTarget() (NO LOCK)
    //   - Thread 2: UpdateInterruptSystem() -> ProcessInterruptOpportunities() (HOLDS LOCK)
    // This creates DATA RACE when std::shared_mutex throws "resource deadlock would occur"
    // Solution: Use std::recursive_mutex to allow same thread to acquire lock multiple times
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE> _mutex;

    // Constants for WoW 12.0
    static constexpr uint32 DEFAULT_REACTION_TIME = 250;        // 250ms reaction time
    static constexpr float DEFAULT_MAX_RANGE = 30.0f;          // 30 yards max interrupt range
    static constexpr uint32 DEFAULT_SCAN_INTERVAL = 100;       // 100ms scan interval
    static constexpr float TIMING_ACCURACY_TARGET = 0.8f;      // 80% timing accuracy target
    static constexpr uint32 COORDINATION_UPDATE_INTERVAL = 500; // 500ms coordination updates
    static constexpr uint32 TARGET_TRACKING_DURATION = 5000;   // 5 seconds target tracking
    static constexpr uint32 SCHOOL_LOCKOUT_DURATION = 4000;    // 4 seconds default lockout
    static constexpr uint32 INTERRUPT_GCD = 1500;              // 1.5 second GCD
    static constexpr float MYTHIC_PLUS_SCALING = 1.08f;        // 8% scaling per M+ level
    static constexpr uint32 ROTATION_SYNC_INTERVAL = 1000;     // 1 second rotation sync
};

// Interrupt utilities and spell database
class TC_GAME_API InterruptUtils
{
public:
    // Spell analysis utilities
    static InterruptPriority GetSpellInterruptPriority(uint32 spellId);
    static InterruptType GetSpellInterruptType(uint32 spellId);
    static bool IsSpellInterruptible(uint32 spellId);
    static bool IsSpellChanneled(uint32 spellId);
    static float GetSpellDangerRating(uint32 spellId);

    // ========================================================================
    // WoW 12.0 SPELL ATTRIBUTE CHECKS (SpellAttr16 Infrastructure)
    // ========================================================================

    /**
     * @brief Check if a spell has any SpellAttr16 flags
     * @param spellId The spell ID to check
     * @return True if the spell has any Attr16 flags set
     *
     * Note: As of WoW 12.0, all SpellAttr16 flags are UNK (undocumented).
     * This infrastructure is ready for when flags become documented.
     * Known flags: SPELL_ATTR16_UNK0 through SPELL_ATTR16_UNK31
     */
    static bool HasAnySpellAttr16(uint32 spellId);

    /**
     * @brief Check if a spell has a specific SpellAttr16 flag
     * @param spellId The spell ID to check
     * @param attribute The SpellAttr16 flag to check
     * @return True if the spell has the specified attribute
     */
    static bool HasSpellAttr16(uint32 spellId, SpellAttr16 attribute);

    /**
     * @brief Get PvP multiplier for spell interrupt priority assessment
     * @param spellId The spell ID to check
     * @param effectIndex The effect index (default 0)
     * @return PvP multiplier value (1.0 = no reduction, <1.0 = reduced in PvP)
     *
     * Uses SpellPvpModifier data from SpellEffectEntry to determine
     * if a spell is particularly dangerous in PvP (high multiplier)
     * or already reduced (low multiplier).
     */
    static float GetPvPInterruptMultiplier(uint32 spellId, uint8 effectIndex = 0);

    /**
     * @brief Check if spell should have increased interrupt priority in PvP
     * @param spellId The spell ID to check
     * @return True if the spell should be prioritized for interruption in PvP
     *
     * Spells with PvpMultiplier > 0.9 (less than 10% reduction) are considered
     * particularly dangerous in PvP and should be higher priority for interrupts.
     */
    static bool IsPvPHighPriorityInterrupt(uint32 spellId);

    // Class-specific interrupt utilities
    static ::std::vector<uint32> GetClassInterruptSpells(uint8 playerClass);
    static uint32 GetBestInterruptSpell(uint8 playerClass, InterruptType type);
    static bool CanClassInterrupt(uint8 playerClass);
    static float GetClassInterruptRange(uint8 playerClass);

    // Timing calculation utilities
    static float CalculateOptimalInterruptTiming(float totalCastTime, float reactionTime, float executionTime);
    static bool IsInterruptTimingFeasible(float castTimeRemaining, float executionTime, float marginError = 0.1f);
    static float PredictSpellCompletionTime(Unit* caster);
    static uint32 GetInterruptGracePeriod(uint32 spellId);

    // Priority assessment utilities
    static float CalculateHealingThreat(const SpellInfo* spellInfo, Unit* caster);
    static float CalculateDamageThreat(const SpellInfo* spellInfo, Unit* caster);
    static float CalculateCCThreat(const SpellInfo* spellInfo, Unit* caster);
    static bool IsSpellGameChanging(uint32 spellId);

    // Group coordination utilities
    static bool ShouldPrioritizeInterrupt(Player* bot, const InterruptTarget& target, const ::std::vector<Player*>& group);
    static Player* GetBestInterrupter(const InterruptTarget& target, const ::std::vector<Player*>& group);
    static void DistributeInterruptTargets(const ::std::vector<InterruptTarget>& targets, const ::std::vector<Player*>& group);

    // Database and configuration utilities
    static void LoadInterruptDatabase();
    static void UpdateInterruptPriorities();
    static bool IsInInterruptDatabase(uint32 spellId);
    static InterruptPriority GetConfiguredPriority(uint32 spellId);

    // Emergency interrupt utilities
    static bool RequiresEmergencyInterrupt(uint32 spellId, Unit* caster);
    static ::std::vector<uint32> GetEmergencyInterruptSpells();
    static bool CanForceInterrupt(Player* bot, Unit* target);
};

} // namespace Playerbot