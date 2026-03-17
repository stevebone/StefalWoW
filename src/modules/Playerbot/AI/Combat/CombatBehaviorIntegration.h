/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Define.h"
#include "Threading/LockHierarchy.h"
#include "Position.h"
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <mutex>

class Player;
class Unit;
class Spell;
class SpellInfo;

namespace Playerbot
{
    // Forward declarations for all Phase 3 managers
    class CombatStateAnalyzer;
    class AdaptiveBehaviorManager;
    class TargetManager;
    class InterruptManager;
    class CrowdControlManager;
    class DefensiveManager;
    class MovementIntegration;
    class BotThreatManager;
    class PositionManager;
    class TTKEstimator;

    enum class CombatSituation : uint8;
    enum class BotRole : uint8;
    struct CombatMetrics;

    // Urgency levels for combat actions
    enum class ActionUrgency : uint8
    {
        LOW         = 0,
        NORMAL      = 1,
        HIGH        = 2,
        URGENT      = 3,
        CRITICAL    = 4,
        EMERGENCY   = 5
    };

    // Combat action types for decision making
    enum class CombatActionType : uint8
    {
        NONE            = 0,
        INTERRUPT       = 1,
        CROWD_CONTROL   = 2,
        DEFENSIVE       = 3,
        MOVEMENT        = 4,
        TARGET_SWITCH   = 5,
        EMERGENCY       = 6,
        CONSUMABLE      = 7,
        COOLDOWN        = 8,
        ROTATION        = 9
    };

    // Recommended action from the integration system
    struct RecommendedAction
    {
        CombatActionType type;
        ActionUrgency urgency;
        Unit* target;
        uint32 spellId;
        Position position;
        ::std::string reason;
        uint32 timestamp;

        RecommendedAction() : type(CombatActionType::NONE), urgency(ActionUrgency::NORMAL),
                             target(nullptr), spellId(0), timestamp(0) {}
    };

    // Combat behavior integration - unified interface for ClassAI
    class CombatBehaviorIntegration
    {
    public:
        explicit CombatBehaviorIntegration(Player* bot);
        ~CombatBehaviorIntegration();

        // Main update function - call this from ClassAI
        void Update(uint32 diff);

        // Emergency handling - returns true if emergency action was taken
        bool HandleEmergencies();

        // Quick decision checks for ClassAI
        bool ShouldInterrupt(Unit* target = nullptr);
        bool ShouldInterruptCurrentCast();
        bool NeedsDefensive();
        bool NeedsMovement();
        bool ShouldSwitchTarget();
        bool ShouldUseCrowdControl();
        bool ShouldUseConsumables();
        bool ShouldUseCooldowns();
        bool ShouldSaveCooldowns();

        // Target selection
        Unit* GetPriorityTarget();
        Unit* GetInterruptTarget();
        Unit* GetCrowdControlTarget();
        bool ShouldFocusAdd();
        bool ShouldAOE();

        // Movement decisions
        Position GetOptimalPosition();
        float GetOptimalRange(Unit* target = nullptr);
        bool ShouldMoveToPosition(const Position& pos);
        bool IsPositionSafe(const Position& pos);
        bool NeedsRepositioning();

        // Resource management
        bool CanAffordSpell(uint32 spellId);
        bool ShouldConserveMana();
        bool IsResourceLow();

        // Combat state queries
        CombatSituation GetCurrentSituation() const;
        const CombatMetrics& GetCombatMetrics() const;
        BotRole GetCurrentRole() const;
        bool IsEmergencyMode() const;
        bool IsSurvivalMode() const;

        // Strategy queries
        bool IsStrategyActive(uint32 flag) const;
        uint32 GetActiveStrategies() const;
        void ActivateStrategy(uint32 flags);
        void DeactivateStrategy(uint32 flags);

        // Recommended action system
        RecommendedAction GetNextAction();
        bool HasPendingAction() const;
        void ClearPendingActions();
        void RecordActionResult(const RecommendedAction& action, bool success);

        // Performance and debugging
        uint32 GetUpdateTime() const { return _lastUpdateTime; }
        uint32 GetAverageUpdateTime() const;
        void EnableDetailedLogging(bool enable) { _detailedLogging = enable; }
        void DumpState() const;

        // Manager access (for advanced users)
        CombatStateAnalyzer* GetStateAnalyzer() { return _stateAnalyzer.get(); }
        AdaptiveBehaviorManager* GetBehaviorManager() { return _behaviorManager.get(); }
        TargetManager* GetTargetManager() { return _targetManager.get(); }
        InterruptManager* GetInterruptManager() { return _interruptManager.get(); }
        CrowdControlManager* GetCrowdControlManager() { return _crowdControlManager.get(); }
        DefensiveManager* GetDefensiveManager() { return _defensiveManager.get(); }
        MovementIntegration* GetMovementIntegration() { return _movementIntegration.get(); }
        TTKEstimator* GetTTKEstimator() { return _ttkEstimator.get(); }

        // Reset and cleanup
        void Reset();
        void OnCombatStart();
        void OnCombatEnd();

    private:
        // Internal update functions
        void UpdateManagers(uint32 diff);
        void UpdatePriorities();
        void GenerateRecommendations();
        void PrioritizeActions();

        // Action evaluation
        ActionUrgency EvaluateInterruptPriority(Unit* target);
        ActionUrgency EvaluateDefensivePriority();
        ActionUrgency EvaluateMovementPriority();
        ActionUrgency EvaluateTargetSwitchPriority();

        // Helper functions
        bool IsManagerReady() const;
        void LogAction(const RecommendedAction& action, bool executed);
        float CalculateActionScore(const RecommendedAction& action) const;

        // Member variables
        Player* _bot;

        // Core infrastructure (created first - dependencies for other managers)
        std::unique_ptr<BotThreatManager> _threatManager;
        std::unique_ptr<PositionManager> _positionManager;

        // Manager instances
        ::std::unique_ptr<CombatStateAnalyzer> _stateAnalyzer;
        ::std::unique_ptr<AdaptiveBehaviorManager> _behaviorManager;
        ::std::unique_ptr<TargetManager> _targetManager;
        ::std::unique_ptr<InterruptManager> _interruptManager;
        ::std::unique_ptr<CrowdControlManager> _crowdControlManager;
        ::std::unique_ptr<DefensiveManager> _defensiveManager;
        ::std::unique_ptr<MovementIntegration> _movementIntegration;
        ::std::unique_ptr<TTKEstimator> _ttkEstimator;

        // Action queue and recommendations
        ::std::vector<RecommendedAction> _actionQueue;
        mutable Playerbot::OrderedMutex<Playerbot::LockOrder::BOT_AI_STATE> _actionQueueMutex;  // Protects _actionQueue from concurrent access
        RecommendedAction _currentAction;
        uint32 _lastActionTime;

        // State tracking
        bool _inCombat;
        bool _emergencyMode;
        bool _survivalMode;
        uint32 _combatStartTime;

        // Performance tracking
        uint32 _updateTimer;
        uint32 _lastUpdateTime;
        uint32 _totalUpdateTime;
        uint32 _updateCount;
        bool _detailedLogging;

        // Success tracking
        uint32 _successfulActions;
        uint32 _failedActions;
        ::std::map<CombatActionType, uint32> _actionCounts;
        ::std::map<CombatActionType, uint32> _actionSuccesses;
    };

    // Inline helper functions for ClassAI integration
    inline bool RequiresImmediateAction(ActionUrgency urgency)
    {
        return urgency >= ActionUrgency::URGENT;
    }

    inline bool IsEmergencyAction(ActionUrgency urgency)
    {
        return urgency >= ActionUrgency::EMERGENCY;
    }

    inline const char* GetActionName(CombatActionType action)
    {
        switch (action)
        {
            case CombatActionType::INTERRUPT: return "Interrupt";
            case CombatActionType::CROWD_CONTROL: return "Crowd Control";
            case CombatActionType::DEFENSIVE: return "Defensive";
            case CombatActionType::MOVEMENT: return "Movement";
            case CombatActionType::TARGET_SWITCH: return "Target Switch";
            case CombatActionType::EMERGENCY: return "Emergency";
            case CombatActionType::CONSUMABLE: return "Consumable";
            case CombatActionType::COOLDOWN: return "Cooldown";
            case CombatActionType::ROTATION: return "Rotation";
            default: return "None";
        }
    }

    inline const char* GetUrgencyName(ActionUrgency urgency)
    {
        switch (urgency)
        {
            case ActionUrgency::LOW: return "Low";
            case ActionUrgency::NORMAL: return "Normal";
            case ActionUrgency::HIGH: return "High";
            case ActionUrgency::URGENT: return "Urgent";
            case ActionUrgency::CRITICAL: return "Critical";
            case ActionUrgency::EMERGENCY: return "Emergency";
            default: return "Unknown";
        }
    }

} // namespace Playerbot