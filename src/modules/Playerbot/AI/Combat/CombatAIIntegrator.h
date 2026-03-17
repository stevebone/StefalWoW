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
#include "Player.h"
#include "Unit.h"
#include "ObjectGuid.h"
#include <memory>
#include <vector>
#include <chrono>
#include <atomic>
#include <shared_mutex>

namespace Playerbot
{

// Forward declarations
class BotAI;
class ClassAI;
class RoleBasedCombatPositioning;
class InterruptCoordinatorFixed;
using InterruptCoordinator = InterruptCoordinatorFixed;
class ThreatCoordinator;
class FormationManager;
class TargetSelector;
class PathfindingManager;
class LineOfSightManager;
class ObstacleAvoidanceManager;
class KitingManager;
class InterruptDatabase;
class InterruptAwareness;
class MechanicAwareness;
class ThreatAbilities;

// Combat system performance metrics
struct CombatMetrics
{
    // CPU metrics
    ::std::atomic<uint64_t> totalCpuCycles{0};
    ::std::atomic<uint32_t> updateCount{0};
    ::std::atomic<float> avgCpuPercent{0.0f};

    // Memory metrics
    ::std::atomic<size_t> memoryUsed{0};
    ::std::atomic<size_t> peakMemory{0};

    // Component timing
    ::std::chrono::microseconds positioningTime{0};
    ::std::chrono::microseconds interruptTime{0};
    ::std::chrono::microseconds threatTime{0};
    ::std::chrono::microseconds targetingTime{0};

    // Combat statistics
    ::std::atomic<uint32_t> interruptsSuccessful{0};
    ::std::atomic<uint32_t> interruptsAttempted{0};
    ::std::atomic<uint32_t> positionChanges{0};
    ::std::atomic<uint32_t> threatAdjustments{0};

    void Reset()
    {
        totalCpuCycles = 0;
        updateCount = 0;
        avgCpuPercent = 0.0f;
        memoryUsed = 0;
        peakMemory = 0;
        positioningTime = ::std::chrono::microseconds{0};
        interruptTime = ::std::chrono::microseconds{0};
        threatTime = ::std::chrono::microseconds{0};
        targetingTime = ::std::chrono::microseconds{0};
        interruptsSuccessful = 0;
        interruptsAttempted = 0;
        positionChanges = 0;
        threatAdjustments = 0;
    }
};

// Combat AI integration configuration
struct CombatAIConfig
{
    // Feature toggles
    bool enablePositioning = true;
    bool enableInterrupts = true;
    bool enableThreatManagement = true;
    bool enableTargeting = true;
    bool enableFormations = true;
    bool enablePathfinding = true;
    bool enableKiting = true;
    bool enableMechanicsHandling = true;

    // Performance limits
    uint32_t maxCpuMicrosPerUpdate = 100;  // 0.1ms = 0.01% CPU at 100Hz
    size_t maxMemoryBytes = 10485760;       // 10MB limit
    uint32_t updateIntervalMs = 100;        // 100ms default update rate

    // Combat behavior tuning
    float positionUpdateThreshold = 5.0f;   // Min distance to trigger reposition
    uint32_t interruptReactionTimeMs = 200; // Reaction time for interrupts
    float threatUpdateThreshold = 10.0f;    // Min threat change to update
    uint32_t targetSwitchCooldownMs = 1000; // Cooldown between target switches

    // Group coordination
    bool enableGroupCoordination = true;
    uint32_t maxGroupSize = 40;            // Max raid size
    float groupPositionSpread = 10.0f;     // Max spread for group positioning
};

// Combat AI state machine
enum class CombatPhase
{
    NONE,
    ENGAGING,       // Moving to combat
    OPENING,        // Initial combat actions
    SUSTAINED,      // Main combat rotation
    EXECUTE,        // Target low health
    DEFENSIVE,      // Bot under pressure
    KITING,         // Kiting enemies
    REPOSITIONING,  // Moving to better position
    INTERRUPTING,   // Executing interrupt
    RECOVERING      // Post-combat recovery
};

// Integration result for debugging
struct IntegrationResult
{
    bool success = false;
    ::std::string errorMessage;
    CombatPhase phase = CombatPhase::NONE;
    uint32_t actionsExecuted = 0;
    ::std::chrono::microseconds executionTime{0};
};

// Main combat AI integration class
class TC_GAME_API CombatAIIntegrator
{
public:
    explicit CombatAIIntegrator(Player* bot);
    ~CombatAIIntegrator();

    // Core integration interface
    IntegrationResult Update(uint32 diff);
    void Reset();
    void OnCombatStart(Unit* target);
    void OnCombatEnd();
    void OnTargetChanged(Unit* newTarget);

    // Component access (disabled - components not initialized)
    // RoleBasedCombatPositioning* GetPositioning() const { return _positioning.get(); }
    // InterruptCoordinator* GetInterruptCoordinator() const { return _interruptCoordinator.get(); }
    // ThreatCoordinator* GetThreatCoordinator() const { return _threatCoordinator.get(); }
    // FormationManager* GetFormationManager() const { return _formationManager.get(); }
    // TargetSelector* GetTargetSelector() const { return _targetSelector.get(); }

    // Configuration
    void SetConfig(CombatAIConfig const& config) { _config = config; }
    CombatAIConfig const& GetConfig() const { return _config; }

    // Performance metrics
    CombatMetrics const& GetMetrics() const { return _metrics; }
    void ResetMetrics() { _metrics.Reset(); }

    // Combat state
    CombatPhase GetPhase() const { return _currentPhase; }
    bool IsInCombat() const { return _inCombat; }
    Unit* GetCurrentTarget() const { return _currentTarget; }

    // Group coordination
    void SetGroup(Group* group);
    void UpdateGroupCoordination();

    // Class-specific integration hooks
    void RegisterClassAI(ClassAI* classAI) { _classAI = classAI; }
    void UnregisterClassAI() { _classAI = nullptr; }

private:
    // Internal update methods
    void UpdateCombatPhase(uint32 diff);
    void UpdatePositioning(uint32 diff);
    void UpdateInterrupts(uint32 diff);
    void UpdateThreatManagement(uint32 diff);
    void UpdateTargeting(uint32 diff);
    void UpdateFormation(uint32 diff);
    void UpdatePathfinding(uint32 diff);

    // Phase-specific behaviors
    void HandleEngagingPhase();
    void HandleOpeningPhase();
    void HandleSustainedPhase();
    void HandleExecutePhase();
    void HandleDefensivePhase();
    void HandleKitingPhase();
    void HandleRepositioningPhase();
    void HandleInterruptingPhase();
    void HandleRecoveringPhase();

    // Utility methods
    bool ShouldUpdatePosition();
    bool ShouldInterrupt();
    bool ShouldAdjustThreat();
    bool ShouldSwitchTarget();
    bool ShouldKite();

    // Performance monitoring
    void StartMetricCapture();
    void EndMetricCapture(::std::chrono::microseconds elapsed);
    bool IsWithinPerformanceLimits() const;

    // Memory management
    void ValidateMemoryUsage();
    void CompactMemory();

private:
    // Bot reference
    Player* _bot;
    ClassAI* _classAI;

    // Note: Phase 2 Combat Components disabled due to API incompatibilities
    // Component integration will be added once APIs are aligned
    // ::std::unique_ptr<RoleBasedCombatPositioning> _positioning;
    // ::std::unique_ptr<InterruptCoordinator> _interruptCoordinator;
    // ::std::unique_ptr<ThreatCoordinator> _threatCoordinator;
    // ::std::unique_ptr<FormationManager> _formationManager;
    // ::std::unique_ptr<TargetSelector> _targetSelector;
    // ::std::unique_ptr<PathfindingManager> _pathfinding;
    // ::std::unique_ptr<LineOfSightManager> _losManager;
    // ::std::unique_ptr<ObstacleAvoidanceManager> _obstacleAvoidance;
    // ::std::unique_ptr<KitingManager> _kitingManager;
    // ::std::unique_ptr<InterruptDatabase> _interruptDB;
    // ::std::unique_ptr<InterruptAwareness> _interruptAwareness;
    // ::std::unique_ptr<MechanicAwareness> _mechanicAwareness;
    // ::std::unique_ptr<ThreatAbilities> _threatAbilities;

    // Combat state
    ::std::atomic<bool> _inCombat{false};
    CombatPhase _currentPhase{CombatPhase::NONE};
    Unit* _currentTarget{nullptr};
    Group* _group{nullptr};

    // Timing
    uint32 _lastUpdate{0};
    uint32 _combatStartTime{0};
    uint32 _phaseStartTime{0};
    uint32 _lastPositionUpdate{0};
    uint32 _lastInterruptCheck{0};
    uint32 _lastThreatUpdate{0};

    // Configuration and metrics
    CombatAIConfig _config;
    CombatMetrics _metrics;

    // Thread safety
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE> _mutex;

    // Performance optimization
    static constexpr uint32 MIN_UPDATE_INTERVAL = 50;  // 50ms minimum between updates
    static constexpr uint32 MAX_UPDATE_INTERVAL = 500; // 500ms maximum between updates
};

// Factory for creating integrated combat AI
class TC_GAME_API CombatAIFactory
{
public:
    static ::std::unique_ptr<CombatAIIntegrator> CreateCombatAI(Player* bot);
    static ::std::unique_ptr<CombatAIIntegrator> CreateCombatAI(Player* bot, CombatAIConfig const& config);

    // Specialized creation for different roles
    static ::std::unique_ptr<CombatAIIntegrator> CreateTankCombatAI(Player* bot);
    static ::std::unique_ptr<CombatAIIntegrator> CreateHealerCombatAI(Player* bot);
    static ::std::unique_ptr<CombatAIIntegrator> CreateMeleeDPSCombatAI(Player* bot);
    static ::std::unique_ptr<CombatAIIntegrator> CreateRangedDPSCombatAI(Player* bot);
};

} // namespace Playerbot