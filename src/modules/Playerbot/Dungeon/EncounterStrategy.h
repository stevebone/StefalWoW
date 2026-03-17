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
#include "DungeonBehavior.h"
#include "Player.h"
#include "Group.h"
#include "Unit.h"
#include <unordered_map>
#include <vector>
#include <functional>
#include <atomic>
#include <mutex>

namespace Playerbot
{

// Forward declaration of strategy callback type
using StrategyCallback = ::std::function<void(Player*, Group*, DungeonEncounter const&)>;

/**
 * @brief Tank strategy for encounters
 */
struct TankStrategy
{
    bool shouldTaunt{true};
    bool shouldKite{false};
    bool useCooldowns{true};
    Position preferredPosition;
    uint32 tauntSpellId{0};
    float threatThreshold{1.1f};
    StrategyCallback positioningStrategy{nullptr};
    Position optimalPosition;
    bool requiresMovement{false};
};

/**
 * @brief Healer strategy for encounters
 */
struct HealerStrategy
{
    bool prioritizeTanks{true};
    bool useHoTs{true};
    bool conserveMana{false};
    float emergencyThreshold{0.3f};
    float normalThreshold{0.7f};
    uint32 emergencySpellId{0};
    StrategyCallback healingPriorityStrategy{nullptr};
    Position safePosition;
    float healingThreshold{0.7f};
    bool requiresMovement{false};
};

/**
 * @brief DPS strategy for encounters
 */
struct DpsStrategy
{
    bool prioritizeBoss{true};
    bool useAoE{false};
    bool useCooldowns{true};
    float switchTargetThreshold{0.2f};
    uint32 burstSpellId{0};
    StrategyCallback damageOptimizationStrategy{nullptr};
    Position optimalPosition;
    float threatLimit{0.9f};
    bool canMoveDuringCast{false};
};

/**
 * @brief Strategy execution metrics
 */
struct StrategyMetrics
{
    uint32 strategiesExecuted{0};
    uint32 strategiesSuccessful{0};
    uint32 mechanicsHandled{0};
    uint32 mechanicsSuccessful{0};
    float averageExecutionTime{0.0f};
    float strategySuccessRate{0.0f};
    float mechanicSuccessRate{0.0f};
    uint32 adaptationsPerformed{0};
};

/**
 * @brief Static generic library for dungeon encounter mechanics
 *
 * This class provides STATIC GENERIC METHODS for common dungeon mechanics.
 * These methods are used as fallbacks when no specific dungeon script exists.
 *
 * ARCHITECTURE:
 * - All mechanic handlers are STATIC PUBLIC methods
 * - Called by DungeonScript base class default implementations
 * - Called by DungeonScriptMgr when no script exists
 * - Called by individual dungeon scripts that want generic behavior
 *
 * THREE-LEVEL FALLBACK SYSTEM:
 * 1. Custom dungeon script (specific implementation)
 * 2. DungeonScript base class (calls these generic methods)
 * 3. Direct call to these generic methods (no script exists)
 */
class TC_GAME_API EncounterStrategy final 
{
public:
    // ============================================================================
    // STATIC GENERIC MECHANIC HANDLERS (Public API)
    // ============================================================================

    /**
     * Generic interrupt handler
     * Prioritizes: Heals > Damage > CC
     * Uses class-specific interrupt spells
     */
    static void HandleGenericInterrupts(::Player* player, ::Creature* boss);

    /**
     * Generic ground effect avoidance
     * Detects dangerous DynamicObjects and moves away
     */
    static void HandleGenericGroundAvoidance(::Player* player, ::Creature* boss);

    /**
     * Generic add kill priority
     * Prioritizes: Healers > Casters > Low Health > Melee
     */
    static void HandleGenericAddPriority(::Player* player, ::Creature* boss);

    /**
     * Generic positioning
     * Tank: Front of boss
     * Melee: Behind boss
     * Ranged: 20-30 yards away
     * Healer: Safe distance
     */
    static void HandleGenericPositioning(::Player* player, ::Creature* boss);

    /**
     * Generic dispel mechanic
     * Prioritizes harmful debuffs on players
     */
    static void HandleGenericDispel(::Player* player, ::Creature* boss);

    /**
     * Generic movement mechanic
     * Maintains optimal range for role
     */
    static void HandleGenericMovement(::Player* player, ::Creature* boss);

    /**
     * Generic spread mechanic
     * Players spread apart by specified distance
     */
    static void HandleGenericSpread(::Player* player, ::Creature* boss, float distance);

    /**
     * Generic stack mechanic
     * Players stack on designated position (usually tank)
     */
    static void HandleGenericStack(::Player* player, ::Creature* boss);

    // ============================================================================
    // LEGACY SINGLETON SYSTEM (Deprecated - kept for compatibility)
    // ============================================================================

    static EncounterStrategy* instance();

    // Core strategy management
    void ExecuteEncounterStrategy(Group* group, uint32 encounterId);
    void UpdateEncounterExecution(Group* group, uint32 encounterId, uint32 diff);
    void HandleEncounterMechanic(Group* group, uint32 encounterId, const ::std::string& mechanic);
    void AdaptStrategyToGroupComposition(Group* group, uint32 encounterId);

    // Phase-based encounter management
    void HandleEncounterPhaseTransition(Group* group, uint32 encounterId, uint32 newPhase);
    void ExecutePhaseStrategy(Group* group, uint32 encounterId, uint32 phase);
    void PrepareForPhaseTransition(Group* group, uint32 encounterId, uint32 upcomingPhase);

    // Mechanic-specific handlers
    void HandleTankSwapMechanic(Group* group, Player* currentTank, Player* newTank);
    void HandleStackingDebuffMechanic(Group* group, Player* affectedPlayer);
    void HandleAoEDamageMechanic(Group* group, const Position& dangerZone, float radius);
    void HandleAddSpawnMechanic(Group* group, const ::std::vector<Unit*>& adds);
    void HandleChanneledSpellMechanic(Group* group, Unit* caster, uint32 spellId);
    void HandleEnrageMechanic(Group* group, Unit* boss, uint32 timeRemaining);

    // Role-specific strategy execution (types defined in IEncounterStrategy.h)
    TankStrategy GetTankStrategy(uint32 encounterId, Player* tank);
    HealerStrategy GetHealerStrategy(uint32 encounterId, Player* healer);
    DpsStrategy GetDpsStrategy(uint32 encounterId, Player* dps);

    // Positioning and movement strategies
    void UpdateEncounterPositioning(Group* group, uint32 encounterId);
    void HandleMovementMechanic(Group* group, uint32 encounterId, const ::std::string& mechanic);
    Position CalculateOptimalPosition(Player* player, uint32 encounterId, DungeonRole role);
    void AvoidMechanicAreas(Group* group, const ::std::vector<Position>& dangerAreas);

    // Cooldown and resource management
    void CoordinateGroupCooldowns(Group* group, uint32 encounterId);
    void PlanCooldownUsage(Group* group, const DungeonEncounter& encounter);
    void HandleEmergencyCooldowns(Group* group);
    void OptimizeResourceUsage(Group* group, uint32 encounterId);

    // Public cooldown coordination method (called by DungeonBehavior)
    void CoordinateCooldowns(Group* group, uint32 encounterId);

    // Adaptive strategy system
    void AnalyzeEncounterPerformance(Group* group, uint32 encounterId);
    void AdaptStrategyBasedOnFailures(Group* group, uint32 encounterId);
    void LearnFromSuccessfulEncounters(Group* group, uint32 encounterId);
    void AdjustDifficultyRating(uint32 encounterId, float performanceRating);

    // Encounter-specific strategy implementations
    void ExecuteDeadminesStrategies(Group* group, uint32 encounterId);
    void ExecuteWailingCavernsStrategies(Group* group, uint32 encounterId);
    void ExecuteShadowfangKeepStrategies(Group* group, uint32 encounterId);
    void ExecuteStockadeStrategies(Group* group, uint32 encounterId);
    void ExecuteRazorfenKraulStrategies(Group* group, uint32 encounterId);

    // Performance monitoring (returns StrategyMetrics snapshot from IEncounterStrategy.h)
    StrategyMetrics GetStrategyMetrics(uint32 encounterId);
    StrategyMetrics GetGlobalStrategyMetrics();

    // Configuration and settings
    void SetStrategyComplexity(uint32 encounterId, float complexity); // 0.0 = simple, 1.0 = complex
    void EnableAdaptiveStrategies(bool enable) { _adaptiveStrategiesEnabled = enable; }
    void SetMechanicResponseTime(uint32 responseTimeMs) { _mechanicResponseTime = responseTimeMs; }

private:
    EncounterStrategy();
    ~EncounterStrategy() = default;

    // Internal atomic metrics for thread-safe storage
    struct AtomicStrategyMetrics
    {
        ::std::atomic<uint32> strategiesExecuted{0};
        ::std::atomic<uint32> strategiesSuccessful{0};
        ::std::atomic<uint32> mechanicsHandled{0};
        ::std::atomic<uint32> mechanicsSuccessful{0};
        ::std::atomic<float> averageExecutionTime{300000.0f};
        ::std::atomic<float> strategySuccessRate{0.85f};
        ::std::atomic<float> mechanicSuccessRate{0.9f};
        ::std::atomic<uint32> adaptationsPerformed{0};

        void Reset()
        {
            strategiesExecuted = 0; strategiesSuccessful = 0; mechanicsHandled = 0;
            mechanicsSuccessful = 0; averageExecutionTime = 300000.0f;
            strategySuccessRate = 0.85f; mechanicSuccessRate = 0.9f;
            adaptationsPerformed = 0;
        }

        // Convert to snapshot for external use
        StrategyMetrics ToSnapshot() const
        {
            StrategyMetrics snapshot;
            snapshot.strategiesExecuted = strategiesExecuted.load();
            snapshot.strategiesSuccessful = strategiesSuccessful.load();
            snapshot.mechanicsHandled = mechanicsHandled.load();
            snapshot.mechanicsSuccessful = mechanicsSuccessful.load();
            snapshot.averageExecutionTime = averageExecutionTime.load();
            snapshot.strategySuccessRate = strategySuccessRate.load();
            snapshot.mechanicSuccessRate = mechanicSuccessRate.load();
            snapshot.adaptationsPerformed = adaptationsPerformed.load();
            return snapshot;
        }
    };

    // Strategy database
    ::std::unordered_map<uint32, TankStrategy> _tankStrategies; // encounterId -> strategy
    ::std::unordered_map<uint32, HealerStrategy> _healerStrategies;
    ::std::unordered_map<uint32, DpsStrategy> _dpsStrategies;
    ::std::unordered_map<uint32, AtomicStrategyMetrics> _encounterMetrics;
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BEHAVIOR_MANAGER> _strategyMutex;

    // Encounter mechanics database
    struct EncounterMechanic
    {
        ::std::string mechanicName;
        ::std::string description;
        uint32 triggerCondition;
        uint32 duration;
        float dangerLevel;
        ::std::vector<::std::string> counterMeasures;
        ::std::function<void(Group*, const EncounterMechanic&)> handler;

        EncounterMechanic(const ::std::string& name, const ::std::string& desc)
            : mechanicName(name), description(desc), triggerCondition(0)
            , duration(0), dangerLevel(5.0f) {}
    };

    ::std::unordered_map<uint32, ::std::vector<EncounterMechanic>> _encounterMechanics; // encounterId -> mechanics

    // Adaptive learning system
    struct StrategyLearningData
    {
        ::std::unordered_map<uint32, uint32> mechanicFailures; // mechanicHash -> failure count
        ::std::unordered_map<uint32, uint32> mechanicSuccesses; // mechanicHash -> success count
        ::std::unordered_map<uint32, float> strategyEffectiveness; // strategyHash -> effectiveness
        uint32 totalEncountersAttempted;
        uint32 totalEncountersSuccessful;
        uint32 lastLearningUpdate;

        StrategyLearningData() : totalEncountersAttempted(0), totalEncountersSuccessful(0)
            , lastLearningUpdate(GameTime::GetGameTimeMS()) {}
    };

    ::std::unordered_map<uint32, StrategyLearningData> _learningData; // encounterId -> learning data

    // Configuration
    ::std::atomic<bool> _adaptiveStrategiesEnabled{true};
    ::std::atomic<uint32> _mechanicResponseTime{2000}; // 2 seconds
    ::std::atomic<float> _strategyComplexity{0.7f}; // 70% complexity

    // Global metrics (atomic for thread safety)
    AtomicStrategyMetrics _globalMetrics;

    // Strategy initialization
    void InitializeStrategyDatabase();
    void LoadTankStrategies();
    void LoadHealerStrategies();
    void LoadDpsStrategies();
    void LoadEncounterMechanics();

    // Strategy execution helpers
    void ExecuteRoleStrategy(Player* player, uint32 encounterId, DungeonRole role);
    void HandleSpecificMechanic(Group* group, const EncounterMechanic& mechanic);
    void CoordinateGroupResponse(Group* group, const ::std::string& mechanic);
    void ValidateStrategyExecution(Group* group, uint32 encounterId);

    // Generic mechanic handlers (internal)
    void HandleTankSwapGeneric(Group* group);
    void HandleAoEDamageGeneric(Group* group);
    void HandleAddSpawnsGeneric(Group* group);
    void HandleStackingDebuffGeneric(Group* group);

    // Role determination helper
    static DungeonRole DeterminePlayerRole(Player* player);

    // Positioning algorithms
    Position CalculateTankPosition(uint32 encounterId, Group* group);
    Position CalculateHealerPosition(uint32 encounterId, Group* group);
    Position CalculateDpsPosition(uint32 encounterId, Group* group, bool isMelee);
    void UpdateGroupFormation(Group* group, uint32 encounterId);

    // Learning and adaptation
    void UpdateLearningData(uint32 encounterId, const ::std::string& mechanic, bool wasSuccessful);
    void AdaptStrategyComplexity(uint32 encounterId);
    void OptimizeStrategyBasedOnLearning(uint32 encounterId);
    uint32 GenerateMechanicHash(const ::std::string& mechanic);

    // Performance analysis
    void AnalyzeGroupPerformance(Group* group, uint32 encounterId);
    void IdentifyPerformanceBottlenecks(Group* group, uint32 encounterId);
    void RecommendStrategyAdjustments(Group* group, uint32 encounterId);

    // Constants
    static constexpr uint32 STRATEGY_UPDATE_INTERVAL = 1000; // 1 second
    static constexpr uint32 DEFAULT_MECHANIC_RESPONSE_TIME = 2000; // 2 seconds
    static constexpr float MECHANIC_SUCCESS_THRESHOLD = 0.8f; // 80% success rate
    static constexpr uint32 LEARNING_UPDATE_INTERVAL = 300000; // 5 minutes
    static constexpr float MIN_STRATEGY_EFFECTIVENESS = 0.3f;
    static constexpr uint32 MAX_STRATEGY_ADAPTATIONS = 10;
    static constexpr float POSITIONING_TOLERANCE = 3.0f; // 3 yards
    static constexpr uint32 COOLDOWN_COORDINATION_WINDOW = 5000; // 5 seconds
};

/**
 * ENCOUNTER STRATEGY EXAMPLES:
 *
 * DEADMINES - VANCLEEF:
 * - Phase 1: Tank positioning, add management
 * - Phase 2: Movement coordination, ground effects
 * - Mechanics: Add spawns, ground fire, knockback
 *
 * WAILING CAVERNS - MUTANUS:
 * - Single phase encounter
 * - Mechanics: Sleep effects, positioning
 * - Strategy: Dispel priority, movement coordination
 *
 * SHADOWFANG KEEP - ARUGAL:
 * - Phase-based encounter with teleports
 * - Mechanics: Add spawns, teleportation, magic damage
 * - Strategy: Add control, positioning adaptation
 *
 * STORMWIND STOCKADE - HOGGER:
 * - Simple tank and spank with fear mechanic
 * - Mechanics: Fear effects, enrage
 * - Strategy: Fear resistance, threat management
 */

} // namespace Playerbot