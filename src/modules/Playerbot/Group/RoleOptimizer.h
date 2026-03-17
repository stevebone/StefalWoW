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
#include "RoleAssignment.h"
#include "RoleDefinitions.h"
#include "Player.h"
#include "Group.h"
#include <vector>
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <algorithm>

namespace Playerbot
{

/**
 * @brief Advanced role optimization algorithms for group composition
 *
 * This system uses sophisticated algorithms to find optimal role assignments
 * considering multiple factors: effectiveness, synergy, gear, experience, and flexibility.
 */
class TC_GAME_API RoleOptimizer
{
public:
    // Optimization objectives
    enum class OptimizationGoal : uint8
    {
        MAXIMIZE_EFFECTIVENESS  = 0,  // Highest individual role performance
        MAXIMIZE_SYNERGY       = 1,  // Best class/role combinations
        MAXIMIZE_SURVIVABILITY = 2,  // Most robust composition
        MAXIMIZE_FLEXIBILITY   = 3,  // Most adaptable to changes
        MINIMIZE_CONFLICTS     = 4,  // Least role preference conflicts
        BALANCED_APPROACH      = 5   // Weighted combination of all factors
    };

    // Optimization constraints
    struct OptimizationConstraints
    {
        bool strictRoleRequirements;    // Must fill exact role counts
        bool respectPlayerPreferences;  // Honor stated role preferences
        bool allowHybridRoles;         // Enable hybrid role assignments
        bool prioritizeExperience;     // Weight experience heavily
        bool requireMainTank;          // Must have dedicated main tank
        bool requireMainHealer;        // Must have dedicated main healer
        float minEffectivenessThreshold; // Minimum acceptable role effectiveness
        uint32 maxOptimizationTime;    // Maximum time to spend optimizing (ms)

        OptimizationConstraints()
            : strictRoleRequirements(true), respectPlayerPreferences(true)
            , allowHybridRoles(true), prioritizeExperience(false)
            , requireMainTank(true), requireMainHealer(true)
            , minEffectivenessThreshold(0.4f), maxOptimizationTime(1000) {} // 1 second
    };

    // Optimization result
    struct OptimizationResult
    {
        ::std::unordered_map<uint32, GroupRole> roleAssignments; // playerGuid -> role
        float totalScore;
        float effectivenessScore;
        float synergyScore;
        float flexibilityScore;
        float constraintViolationPenalty;
        bool isValid;
        uint32 optimizationTimeMs;
        ::std::vector<::std::string> warnings;
        ::std::vector<::std::string> recommendations;

        OptimizationResult() : totalScore(0.0f), effectivenessScore(0.0f)
            , synergyScore(0.0f), flexibilityScore(0.0f), constraintViolationPenalty(0.0f)
            , isValid(false), optimizationTimeMs(0) {}
    };

    // Core optimization methods
    static OptimizationResult OptimizeGroupRoles(
        Group* group,
        OptimizationGoal goal = OptimizationGoal::BALANCED_APPROACH,
        const OptimizationConstraints& constraints = OptimizationConstraints()
    );

    static OptimizationResult OptimizeRolesForContent(
        Group* group,
        uint32 contentId,
        bool isDungeon = true,
        const OptimizationConstraints& constraints = OptimizationConstraints()
    );

    // Advanced optimization algorithms
    static OptimizationResult GeneticAlgorithmOptimization(
        const ::std::vector<Player*>& players,
        const GroupComposition& targetComposition,
        const OptimizationConstraints& constraints
    );

    static OptimizationResult SimulatedAnnealingOptimization(
        const ::std::vector<Player*>& players,
        const GroupComposition& targetComposition,
        const OptimizationConstraints& constraints
    );

    static OptimizationResult GreedyOptimization(
        const ::std::vector<Player*>& players,
        const GroupComposition& targetComposition,
        const OptimizationConstraints& constraints
    );

    // Scoring and evaluation
    static float CalculateCompositionScore(
        const ::std::unordered_map<uint32, GroupRole>& assignments,
        const ::std::vector<Player*>& players,
        OptimizationGoal goal
    );

    static float CalculateRoleSynergy(
        const ::std::unordered_map<uint32, GroupRole>& assignments,
        const ::std::vector<Player*>& players
    );

    static float CalculateFlexibilityScore(
        const ::std::unordered_map<uint32, GroupRole>& assignments,
        const ::std::vector<Player*>& players
    );

    // Constraint validation
    static bool ValidateConstraints(
        const ::std::unordered_map<uint32, GroupRole>& assignments,
        const ::std::vector<Player*>& players,
        const OptimizationConstraints& constraints
    );

    static float CalculateConstraintViolationPenalty(
        const ::std::unordered_map<uint32, GroupRole>& assignments,
        const ::std::vector<Player*>& players,
        const OptimizationConstraints& constraints
    );

    // Role conflict resolution
    static ::std::vector<OptimizationResult> ResolveRoleConflicts(
        Group* group,
        const ::std::vector<::std::pair<uint32, GroupRole>>& conflicts
    );

    static OptimizationResult HandleEmergencyReassignment(
        Group* group,
        uint32 leavingPlayerGuid,
        const ::std::vector<uint32>& availableReplacements
    );

    // Predictive optimization
    static ::std::vector<GroupRole> PredictOptimalRoles(
        const ::std::vector<Player*>& players,
        uint32 targetContentId
    );

    static float PredictGroupPerformance(
        const ::std::unordered_map<uint32, GroupRole>& assignments,
        const ::std::vector<Player*>& players,
        uint32 contentId
    );

private:
    // Genetic Algorithm components
    struct Chromosome
    {
        ::std::unordered_map<uint32, GroupRole> genes; // playerGuid -> role
        float fitness;

        Chromosome() : fitness(0.0f) {}
    };

    static ::std::vector<Chromosome> InitializePopulation(
        const ::std::vector<Player*>& players,
        uint32 populationSize
    );

    static Chromosome Crossover(const Chromosome& parent1, const Chromosome& parent2);
    static void Mutate(Chromosome& chromosome, float mutationRate);
    static void EvaluateFitness(Chromosome& chromosome, const ::std::vector<Player*>& players, OptimizationGoal goal);

    // Simulated Annealing components
    static ::std::unordered_map<uint32, GroupRole> GenerateRandomSolution(const ::std::vector<Player*>& players);
    static ::std::unordered_map<uint32, GroupRole> GenerateNeighborSolution(
        const ::std::unordered_map<uint32, GroupRole>& current,
        const ::std::vector<Player*>& players
    );
    static bool AcceptSolution(float currentScore, float newScore, float temperature);

    // Optimization utilities
    static ::std::vector<::std::pair<uint32, ::std::vector<GroupRole>>> GetPlayerRoleOptions(
        const ::std::vector<Player*>& players,
        const OptimizationConstraints& constraints
    );

    static bool IsValidRoleAssignment(
        uint32 playerGuid,
        GroupRole role,
        const ::std::vector<Player*>& players,
        const OptimizationConstraints& constraints
    );

    static void ApplyHeuristicImprovements(
        ::std::unordered_map<uint32, GroupRole>& assignments,
        const ::std::vector<Player*>& players
    );

    // Scoring components
    static float CalculateIndividualEffectiveness(
        uint32 playerGuid,
        GroupRole role,
        const ::std::vector<Player*>& players
    );

    static float CalculateClassSynergy(
        const ::std::vector<::std::pair<uint8, GroupRole>>& classRolePairs
    );

    static float CalculateRoleBalance(
        const ::std::unordered_map<GroupRole, uint32>& roleCounts
    );

    // Content-specific optimization
    static GroupComposition GetOptimalCompositionForContent(uint32 contentId, bool isDungeon);
    static ::std::unordered_map<GroupRole, float> GetRoleImportanceForContent(uint32 contentId, bool isDungeon);

    // Performance monitoring
    struct OptimizationMetrics
    {
        ::std::atomic<uint32> totalOptimizations{0};
        ::std::atomic<uint32> successfulOptimizations{0};
        ::std::atomic<float> averageOptimizationTime{500.0f};
        ::std::atomic<float> averageScoreImprovement{0.2f};
        ::std::atomic<uint32> constraintViolations{0};

        void Reset()
        {
            totalOptimizations = 0; successfulOptimizations = 0;
            averageOptimizationTime = 500.0f; averageScoreImprovement = 0.2f;
            constraintViolations = 0;
        }
    };

    static OptimizationMetrics _metrics;
    static Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::GROUP_MANAGER> _metricsMutex;

    // Constants for optimization algorithms
    static constexpr uint32 GA_POPULATION_SIZE = 50;
    static constexpr uint32 GA_MAX_GENERATIONS = 100;
    static constexpr float GA_MUTATION_RATE = 0.1f;
    static constexpr float GA_CROSSOVER_RATE = 0.8f;
    static constexpr float SA_INITIAL_TEMPERATURE = 100.0f;
    static constexpr float SA_COOLING_RATE = 0.95f;
    static constexpr uint32 SA_MAX_ITERATIONS = 1000;
    static constexpr float MIN_TEMPERATURE = 0.01f;

    // Scoring weights
    static constexpr float EFFECTIVENESS_WEIGHT = 0.4f;
    static constexpr float SYNERGY_WEIGHT = 0.3f;
    static constexpr float FLEXIBILITY_WEIGHT = 0.2f;
    static constexpr float PREFERENCE_WEIGHT = 0.1f;

    // Role synergy matrix
    static const ::std::unordered_map<GroupRole, ::std::unordered_map<GroupRole, float>> ROLE_SYNERGY_MATRIX;
    static void InitializeSynergyMatrix();

    // Class combination bonuses
    static const ::std::unordered_map<uint8, ::std::unordered_map<uint8, float>> CLASS_SYNERGY_BONUSES;
    static void InitializeClassSynergies();
};

/**
 * OPTIMIZATION ALGORITHMS OVERVIEW:
 *
 * 1. GREEDY ALGORITHM (Fast):
 *    - Assigns best available player to each role in priority order
 *    - Time Complexity: O(n²)
 *    - Use for: Real-time role assignment, simple groups
 *
 * 2. GENETIC ALGORITHM (Balanced):
 *    - Evolves population of role assignments over generations
 *    - Handles complex constraints and multiple objectives
 *    - Time Complexity: O(generations × population × evaluation)
 *    - Use for: Complex groups, multiple constraints
 *
 * 3. SIMULATED ANNEALING (Global Optimization):
 *    - Probabilistically accepts worse solutions to escape local optima
 *    - Gradually "cools" to find global optimum
 *    - Time Complexity: O(iterations × evaluation)
 *    - Use for: Critical content, maximum optimization needed
 *
 * SCORING FACTORS:
 *
 * 1. Individual Effectiveness (40%):
 *    - Class/spec suitability for role
 *    - Gear appropriateness
 *    - Player experience in role
 *
 * 2. Group Synergy (30%):
 *    - Class combinations (e.g., Warrior + Priest)
 *    - Role interactions (e.g., Tank + Healer synergy)
 *    - Buff/debuff complementarity
 *
 * 3. Flexibility (20%):
 *    - Ability to adapt to encounters
 *    - Hybrid role capabilities
 *    - Emergency role switching potential
 *
 * 4. Player Preferences (10%):
 *    - Stated role preferences
 *    - Historical role performance
 *    - Willingness to fill needed roles
 */

} // namespace Playerbot