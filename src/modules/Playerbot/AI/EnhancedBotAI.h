/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "BotAI.h"
#include "Combat/CombatAIIntegrator.h"
#include "ClassAI/ClassAI.h"
#include "Group/GroupRoleEnums.h"
#include <memory>
#include <chrono>

namespace Playerbot
{

// Enhanced AI statistics
struct EnhancedAIStats
{
    uint32 totalUpdates = 0;
    uint32 combatUpdates = 0;
    uint32 soloUpdates = 0;

    ::std::chrono::microseconds totalUpdateTime{0};
    ::std::chrono::microseconds avgUpdateTime{0};
    ::std::chrono::microseconds maxUpdateTime{0};

    float cpuUsagePercent = 0.0f;
    size_t memoryUsageBytes = 0;

    uint32 actionsExecuted = 0;
    uint32 decisionsM = 0;
    uint32 pathCalculations = 0;

    void Reset()
    {
        totalUpdates = 0;
        combatUpdates = 0;
        soloUpdates = 0;
        totalUpdateTime = ::std::chrono::microseconds{0};
        avgUpdateTime = ::std::chrono::microseconds{0};
        maxUpdateTime = ::std::chrono::microseconds{0};
        cpuUsagePercent = 0.0f;
        memoryUsageBytes = 0;
        actionsExecuted = 0;
        decisionsM = 0;
        pathCalculations = 0;
    }
};

// Enhanced BotAI with Phase 2 combat systems integration
class TC_GAME_API EnhancedBotAI : public BotAI
{
public:
    explicit EnhancedBotAI(Player* bot);
    ~EnhancedBotAI();

    // Override base BotAI methods
    void UpdateAI(uint32 diff);
    void Reset();
    void OnDeath();
    void OnRespawn();
    AIUpdateResult UpdateEnhanced(uint32 diff);

    // Combat events
    void OnCombatStart(Unit* target);
    void OnCombatEnd();
    void OnTargetChanged(Unit* newTarget);
    void OnThreatChanged(Unit* target, float threat);
    void OnDamageReceived(Unit* attacker, uint32 damage);
    void OnHealReceived(Unit* healer, uint32 amount);

    // Group events
    void OnGroupJoined(Group* group);
    void OnGroupLeft();
    void OnGroupMemberAdded(Player* member);
    void OnGroupMemberRemoved(Player* member);
    void OnGroupRoleChanged(Playerbot::GroupRole newRole);

    // Movement events
    void OnMovementStarted();
    void OnMovementStopped();
    void OnPositionReached(Position const& pos);
    void OnPathBlocked();

    // Spell events
    void OnSpellCast(SpellInfo const* spell);
    void OnSpellInterrupted(SpellInfo const* spell);
    void OnAuraApplied(AuraEffect const* aura);
    void OnAuraRemoved(AuraEffect const* aura);

    // Component access
    CombatAIIntegrator* GetCombatAI() const { return _combatIntegrator.get(); }
    ClassAI* GetClassAI() const { return _classAI.get(); }

    // Configuration
    void SetCombatConfig(CombatAIConfig const& config);
    CombatAIConfig const& GetCombatConfig() const;

    // Performance metrics
    EnhancedAIStats const& GetStats() const { return _stats; }
    void ResetStats() { _stats.Reset(); }

    // Debug and testing
    void EnableDebugMode(bool enable) { _debugMode = enable; }
    bool IsDebugMode() const { return _debugMode; }
    void LogPerformanceReport();

protected:
    // Internal update methods
    void UpdateCombat(uint32 diff);
    void UpdateSolo(uint32 diff);
    void UpdateMovement(uint32 diff);
    void UpdateGroupCoordination(uint32 diff);
    void UpdateQuesting(uint32 diff);
    void UpdateSocial(uint32 diff);

    // Decision making
    bool ShouldEngageCombat();
    bool ShouldFlee();
    bool ShouldRest();
    bool ShouldLoot();
    bool ShouldFollowGroup();

    // Performance monitoring
    void StartPerformanceCapture();
    void EndPerformanceCapture(::std::chrono::microseconds elapsed);
    bool IsWithinPerformanceBudget() const;
    void ThrottleIfNeeded();

private:
    // Initialize components
    void InitializeCombatAI();
    void InitializeClassAI();
    void LoadConfiguration();

    // State management
    void TransitionToState(BotAIState newState);
    void HandleStateTransition(BotAIState oldState, BotAIState newState);

    // Event processing
    void ProcessCombatEvents(uint32 diff);
    void ProcessMovementEvents(uint32 diff);
    void ProcessGroupEvents(uint32 diff);

    // Cleanup
    void CleanupExpiredData();
    void CompactMemory();

private:
    // Phase 2 Combat System Integration
    ::std::unique_ptr<CombatAIIntegrator> _combatIntegrator;

    // Class-specific AI
    ::std::unique_ptr<ClassAI> _classAI;

    // Current state
    BotAIState _currentState;
    BotAIState _previousState;
    uint32 _stateTransitionTime;

    // Performance tracking
    EnhancedAIStats _stats;
    ::std::chrono::high_resolution_clock::time_point _lastUpdateTime;
    uint32 _updateThrottleMs;

    // Configuration
    bool _debugMode;
    bool _performanceMode;
    uint32 _maxUpdateRateHz;

    // Group coordination
    Group* _currentGroup;
    Playerbot::GroupRole _groupRole;
    ObjectGuid _followTarget;

    // Combat tracking
    bool _inCombat;
    Unit* _primaryTarget;
    ::std::vector<Unit*> _threatList;
    uint32 _combatStartTime;

    // Resource management
    float _currentManaPercent;
    float _currentHealthPercent;
    uint32 _lastRestTime;

    // Update intervals (ms)
    uint32 _combatUpdateInterval;
    uint32 _soloUpdateInterval;
    uint32 _movementUpdateInterval;

    // Timers
    uint32 _lastCombatUpdate;
    uint32 _lastSoloUpdate;
    uint32 _lastMovementUpdate;
    uint32 _lastGroupUpdate;

    // Memory management
    size_t _memoryBudgetBytes;
    uint32 _lastMemoryCheck;
    uint32 _memoryCheckInterval;
};

// Factory for creating enhanced AI instances
class TC_GAME_API EnhancedBotAIFactory
{
public:
    static ::std::unique_ptr<EnhancedBotAI> CreateEnhancedAI(Player* bot);

    // Role-specific creation
    static ::std::unique_ptr<EnhancedBotAI> CreateTankAI(Player* bot);
    static ::std::unique_ptr<EnhancedBotAI> CreateHealerAI(Player* bot);
    static ::std::unique_ptr<EnhancedBotAI> CreateDPSAI(Player* bot);

    // Class-specific creation
    static ::std::unique_ptr<EnhancedBotAI> CreateWarriorAI(Player* bot);
    static ::std::unique_ptr<EnhancedBotAI> CreatePaladinAI(Player* bot);
    static ::std::unique_ptr<EnhancedBotAI> CreateHunterAI(Player* bot);
    static ::std::unique_ptr<EnhancedBotAI> CreateRogueAI(Player* bot);
    static ::std::unique_ptr<EnhancedBotAI> CreatePriestAI(Player* bot);
    static ::std::unique_ptr<EnhancedBotAI> CreateDeathKnightAI(Player* bot);
    static ::std::unique_ptr<EnhancedBotAI> CreateShamanAI(Player* bot);
    static ::std::unique_ptr<EnhancedBotAI> CreateMageAI(Player* bot);
    static ::std::unique_ptr<EnhancedBotAI> CreateWarlockAI(Player* bot);
    static ::std::unique_ptr<EnhancedBotAI> CreateMonkAI(Player* bot);
    static ::std::unique_ptr<EnhancedBotAI> CreateDruidAI(Player* bot);
    static ::std::unique_ptr<EnhancedBotAI> CreateDemonHunterAI(Player* bot);
    static ::std::unique_ptr<EnhancedBotAI> CreateEvokerAI(Player* bot);
};

} // namespace Playerbot