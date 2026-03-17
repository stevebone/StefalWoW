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
#include "BotThreatManager.h"  // Needed for ThreatRole enum
#include "ThreatAbilities.h"   // Needed for ThreatAbilityType, ThreatAbilityData
#include "Core/Events/ICombatEventSubscriber.h"
#include "Core/Events/CombatEventType.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <mutex>

class Player;
class Unit;
class Group;

namespace Playerbot
{
// Forward declaration to reduce header coupling
// Full include is in ThreatCoordinator.cpp
// Note: InterruptCoordinator is a type alias for InterruptCoordinatorFixed
class InterruptCoordinatorFixed;
using InterruptCoordinator = InterruptCoordinatorFixed;
} // namespace Playerbot

namespace Playerbot
{

class BotAI;
class PositionManager;

// Threat coordination states
enum class ThreatState : uint8
{
    STABLE = 0,         // Tank has solid aggro
    UNSTABLE = 1,       // Threat shifting between targets
    CRITICAL = 2,       // Tank lost aggro, emergency
    RECOVERING = 3,     // Recovering from threat loss
    TRANSITIONING = 4   // Tank swap in progress
};

// Bot threat assignment for coordination
struct BotThreatAssignment
{
    ObjectGuid botGuid;
    ObjectGuid targetGuid;
    ThreatRole assignedRole;
    float targetThreatPercent;      // Target threat % to maintain
    float currentThreatPercent;     // Current actual threat %
    bool useAbilities;               // Whether to use threat abilities
    uint32 lastAbilityTime;
    ::std::vector<uint32> availableAbilities;

    BotThreatAssignment() : assignedRole(ThreatRole::UNDEFINED),
                           targetThreatPercent(0.0f), currentThreatPercent(0.0f),
                           useAbilities(false), lastAbilityTime(0) {}
};

// Group threat situation
struct GroupThreatStatus
{
    ThreatState state;
    ObjectGuid primaryTank;
    ObjectGuid offTank;
    ::std::vector<ObjectGuid> activeTargets;
    ::std::unordered_map<ObjectGuid, float> targetThreatLevels;  // target -> highest threat %
    ::std::unordered_map<ObjectGuid, ObjectGuid> targetTanks;    // target -> current tank
    uint32 looseTargets;            // Number of targets not on tank
    uint32 criticalTargets;         // Targets attacking healers/dps
    bool requiresTaunt;
    bool requiresEmergencyResponse;
    ::std::chrono::steady_clock::time_point lastUpdate;

    GroupThreatStatus() : state(ThreatState::STABLE), looseTargets(0),
                         criticalTargets(0), requiresTaunt(false),
                         requiresEmergencyResponse(false) {}
};

// Threat response action
struct ThreatResponseAction
{
    ObjectGuid executorBot;
    ObjectGuid targetUnit;
    uint32 abilitySpellId;
    ThreatAbilityType abilityType;
    ::std::chrono::steady_clock::time_point executeTime;
    uint32 priority;
    bool executed;
    bool succeeded;

    ThreatResponseAction() : abilitySpellId(0), abilityType(ThreatAbilityType::HIGH_THREAT),
                            priority(0), executed(false), succeeded(false) {}

    bool IsReady() const
    {
        return ::std::chrono::steady_clock::now() >= executeTime;
    }
};

// Performance metrics for threat coordination
struct ThreatCoordinationMetrics
{
    ::std::atomic<uint32> threatUpdates{0};
    ::std::atomic<uint32> tauntExecutions{0};
    ::std::atomic<uint32> tauntSuccesses{0};
    ::std::atomic<uint32> threatReductions{0};
    ::std::atomic<uint32> emergencyResponses{0};
    ::std::atomic<uint32> tankSwaps{0};
    ::std::chrono::microseconds averageUpdateTime{0};
    ::std::chrono::microseconds maxUpdateTime{0};
    float averageThreatStability{0.0f};
    float tankControlRate{0.0f};  // % of time tank has aggro

    void Reset()
    {
        threatUpdates = 0;
        tauntExecutions = 0;
        tauntSuccesses = 0;
        threatReductions = 0;
        emergencyResponses = 0;
        tankSwaps = 0;
        averageUpdateTime = ::std::chrono::microseconds{0};
        maxUpdateTime = ::std::chrono::microseconds{0};
        averageThreatStability = 0.0f;
        tankControlRate = 0.0f;
    }
};

/**
 * Advanced threat coordination system for bot groups
 *
 * Manages group-wide threat distribution, tank assignments, and emergency responses
 * to maintain optimal threat control in combat situations.
 *
 * Features:
 * - Real-time threat monitoring across all group members
 * - Automatic taunt rotation and backup assignments
 * - DPS threat throttling to prevent aggro pulls
 * - Healer threat management and protection
 * - Tank swap coordination for encounters
 * - Integration with interrupt and positioning systems
 * - Performance optimization for 5+ bot scenarios
 *
 * Phase 3 Event-Driven Architecture:
 * - Implements ICombatEventSubscriber for threat-related events
 * - Subscribes to: DAMAGE_TAKEN, DAMAGE_DEALT, HEALING_DONE, THREAT_CHANGED, UNIT_DIED
 * - Reduces polling overhead by ~80% through event-driven updates
 * - Maintains 50ms emergency check for safety situations
 */
class TC_GAME_API ThreatCoordinator : public ICombatEventSubscriber
{
public:
    explicit ThreatCoordinator(Group* group = nullptr);
    ~ThreatCoordinator();

    // ========================================================================
    // ICombatEventSubscriber Interface (Phase 3 Event-Driven)
    // ========================================================================

    /**
     * @brief Called when a subscribed combat event occurs
     * Routes events to appropriate threat handlers
     */
    void OnCombatEvent(const CombatEvent& event) override;

    /**
     * @brief Return bitmask of event types this coordinator wants
     * Subscribes to: DAMAGE_TAKEN, DAMAGE_DEALT, HEALING_DONE, THREAT_CHANGED, UNIT_DIED
     */
    CombatEventType GetSubscribedEventTypes() const override;

    /**
     * @brief Normal priority (50) for threat tracking
     * Lower than InterruptCoordinator but higher than logging systems
     */
    int32 GetEventPriority() const override { return 50; }

    /**
     * @brief Filter events to only receive those involving group members
     */
    bool ShouldReceiveEvent(const CombatEvent& event) const;

    /**
     * @brief Subscriber name for debugging
     */
    const char* GetSubscriberName() const override { return "ThreatCoordinator"; }

    // === Core Coordination Interface ===

    // Update coordination system (called from combat update loop)
    void Update(uint32 diff);

    // Register/unregister bots for threat coordination
    void RegisterBot(Player* bot, BotAI* ai);
    void UnregisterBot(ObjectGuid botGuid);
    void UpdateBotRole(ObjectGuid botGuid, ThreatRole role);

    // === Threat Management ===

    // Tank assignment and management
    bool AssignPrimaryTank(ObjectGuid botGuid);
    bool AssignOffTank(ObjectGuid botGuid);
    void InitiateTankSwap(ObjectGuid fromTank, ObjectGuid toTank);

    // Threat response actions
    bool ExecuteTaunt(ObjectGuid tankGuid, Unit* target);
    bool ExecuteThreatReduction(ObjectGuid botGuid, float reductionPercent);
    bool ExecuteThreatTransfer(ObjectGuid fromBot, ObjectGuid toBot, Unit* target);

    // Emergency responses
    void HandleEmergencyThreat(Unit* looseTarget);
    void ProtectHealer(ObjectGuid healerGuid, Unit* attacker);
    void RecoverFromWipe();

    // === Status and Analysis ===

    // Current group threat status
    GroupThreatStatus GetGroupThreatStatus() const;
    bool IsGroupThreatStable() const;
    float GetGroupThreatStability() const;

    // Individual bot status
    BotThreatAssignment const* GetBotAssignment(ObjectGuid botGuid) const;
    float GetBotThreatPercent(ObjectGuid botGuid, Unit* target) const;
    bool IsBotAtThreatCap(ObjectGuid botGuid) const;

    // Target analysis
    Unit* GetHighestThreatTarget() const;
    ::std::vector<Unit*> GetLooseTargets() const;
    ObjectGuid GetTargetTank(Unit* target) const;

    // === Configuration ===

    // Threat thresholds
    void SetTankThreatThreshold(float percent) { _tankThreatThreshold = percent; }
    void SetDpsThreatThreshold(float percent) { _dpsThreatThreshold = percent; }
    void SetHealerThreatThreshold(float percent) { _healerThreatThreshold = percent; }

    // Behavior settings
    void SetAutoTauntEnabled(bool enabled) { _autoTauntEnabled = enabled; }
    void SetThreatThrottlingEnabled(bool enabled) { _threatThrottlingEnabled = enabled; }
    void SetEmergencyResponseEnabled(bool enabled) { _emergencyResponseEnabled = enabled; }

    // Update intervals
    void SetUpdateInterval(uint32 intervalMs) { _updateInterval = intervalMs; }
    void SetEmergencyCheckInterval(uint32 intervalMs) { _emergencyCheckInterval = intervalMs; }

    // === Integration ===

    // Connect with other systems
    void SetInterruptCoordinator(InterruptCoordinator* coordinator) { _interruptCoordinator = coordinator; }
    void SetPositionManager(PositionManager* manager) { _positionManager = manager; }

    // === Performance and Metrics ===

    // Performance metrics
    ThreatCoordinationMetrics const& GetMetrics() const { return _metrics; }
    void ResetMetrics();

    // Debug information
    void LogThreatStatus() const;
    ::std::string GetThreatReport() const;

    // === Advanced Features ===

    // Encounter-specific patterns
    void LoadEncounterThreatPattern(uint32 encounterId);
    void SetCustomTankSwapTiming(uint32 intervalMs) { _tankSwapInterval = intervalMs; }

    // M+ affixes handling
    void HandleSkittishAffix();      // Enemies have reduced aggro radius
    void HandleRagingAffix();         // Enemies enrage at low health
    void HandleBolsteringAffix();     // Enemies buff nearby allies on death

private:
    // === Internal Management ===

    // Core update methods
    void UpdateGroupThreatStatus();
    void UpdateBotAssignments();
    void ProcessThreatResponses();
    void CheckEmergencySituations();

    // Threat calculation and analysis
    float CalculateOptimalThreatPercent(ObjectGuid botGuid, ThreatRole role) const;
    bool ShouldUseThreatAbility(BotThreatAssignment const& assignment) const;
    ThreatAbilityData const* SelectBestThreatAbility(ObjectGuid botGuid, ThreatAbilityType type) const;

    // Tank management
    void UpdateTankAssignments();
    void ValidateTankTargets();
    bool ExecuteTankRotation();

    // DPS/Healer management
    void ThrottleDpsThreat();
    void ManageHealerThreat();
    void BalanceThreatDistribution();

    // Response generation
    void GenerateThreatResponses();
    void QueueThreatResponse(ThreatResponseAction const& action);
    void ExecuteQueuedResponses();

    // Coordination algorithms
    bool CoordinateTauntRotation(Unit* target);
    bool CoordinateTheatReduction(ObjectGuid botGuid);
    bool CoordinateThreatTransfer(Unit* target);

    // Emergency handling
    void InitiateEmergencyProtocol();
    void ExecuteEmergencyTaunt(Unit* target);
    void ExecuteMassTheatReduction();

    // Performance tracking
    void TrackPerformance(::std::chrono::microseconds duration, ::std::string const& operation);
    void UpdateStabilityMetrics();

    // Cleanup
    void CleanupInactiveBots();
    void CleanupExpiredResponses();

    // ========================================================================
    // Event Handlers (Phase 3 Event-Driven Architecture)
    // ========================================================================

    /**
     * @brief Handle DAMAGE_TAKEN event - update threat for damage received
     */
    void HandleDamageTaken(const CombatEvent& event);

    /**
     * @brief Handle DAMAGE_DEALT event - update threat for damage dealt
     */
    void HandleDamageDealt(const CombatEvent& event);

    /**
     * @brief Handle HEALING_DONE event - update threat from healing
     */
    void HandleHealingDone(const CombatEvent& event);

    /**
     * @brief Handle THREAT_CHANGED event - direct threat table update
     */
    void HandleThreatChanged(const CombatEvent& event);

    /**
     * @brief Handle UNIT_DIED event - remove from threat tracking
     */
    void HandleUnitDied(const CombatEvent& event);

    /**
     * @brief Check if a GUID belongs to one of our group members
     */
    bool IsGroupMember(ObjectGuid guid) const;

    // Helper methods
    ThreatRole DetermineRole(Player* bot) const;
    uint32 GetTauntSpellForBot(ObjectGuid botGuid) const;
    bool CanBotTaunt(ObjectGuid botGuid) const;

private:
    // Group reference
    Group* _group;

    // Bot management
    ::std::unordered_map<ObjectGuid, ::std::unique_ptr<BotThreatManager>> _botThreatManagers;
    ::std::unordered_map<ObjectGuid, BotThreatAssignment> _botAssignments;
    ::std::unordered_map<ObjectGuid, BotAI*> _botAIs;

    // Tank assignments
    ObjectGuid _primaryTank;
    ObjectGuid _offTank;
    ::std::vector<ObjectGuid> _backupTanks;

    // Current status
    GroupThreatStatus _groupStatus;
    ::std::vector<ThreatResponseAction> _queuedResponses;

    // Configuration
    float _tankThreatThreshold = 130.0f;      // Tank should maintain 130% threat
    float _dpsThreatThreshold = 90.0f;        // DPS should stay below 90%
    float _healerThreatThreshold = 70.0f;     // Healers should stay below 70%

    bool _autoTauntEnabled = true;
    bool _threatThrottlingEnabled = true;
    bool _emergencyResponseEnabled = true;

    uint32 _updateInterval = 100;             // 100ms standard update
    uint32 _emergencyCheckInterval = 50;      // 50ms emergency check
    uint32 _tankSwapInterval = 0;             // 0 = disabled

    // Integration components
    InterruptCoordinator* _interruptCoordinator = nullptr;
    PositionManager* _positionManager = nullptr;

    // Performance tracking
    mutable ThreatCoordinationMetrics _metrics;
    ::std::chrono::steady_clock::time_point _lastUpdate;
    ::std::chrono::steady_clock::time_point _lastEmergencyCheck;

    // Thread safety
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE> _coordinatorMutex;

    // Phase 3: Event-driven state
    ::std::atomic<bool> _subscribed{false};
    ::std::atomic<uint32> _maintenanceTimer{0};
    ::std::atomic<bool> _threatDataDirty{false};  // Set by event handlers, cleared by Update
    static constexpr uint32 MAINTENANCE_INTERVAL_MS = 500;  // Run maintenance every 500ms

    // Constants
    static constexpr float THREAT_STABILITY_THRESHOLD = 0.8f;   // 80% stability = stable
    static constexpr uint32 MAX_RESPONSE_QUEUE_SIZE = 20;
    static constexpr uint32 TAUNT_GLOBAL_COOLDOWN = 1500;       // 1.5 seconds between taunts
    static constexpr float EMERGENCY_THREAT_THRESHOLD = 150.0f;  // 150% = emergency
};

} // namespace Playerbot