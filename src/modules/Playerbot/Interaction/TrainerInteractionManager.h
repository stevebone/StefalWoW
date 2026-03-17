/*
 * Copyright (C) 2024 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>

class Player;
class Creature;
struct TrainerSpell;

namespace Playerbot
{

/**
 * @class TrainerInteractionManager
 * @brief Manages all trainer interactions for player bots
 *
 * This class provides complete trainer functionality using TrinityCore's
 * trainer system APIs. It handles:
 * - Automatic spell/ability learning based on class and level
 * - Profession training and skill advancement
 * - Talent reset services
 * - Smart spell prioritization (core abilities first)
 * - Gold management for training costs
 *
 * Performance Target: <1ms per training decision
 * Memory Target: <20KB overhead
 *
 * TrinityCore API Integration:
 * - Creature::GetTrainerSpells() - Get trainer's spell list
 * - Player::LearnSpell() - Learn spell directly
 * - Player::HasSpell() - Check if spell is known
 * - Player::GetTrainerSpellState() - Check if spell can be learned
 * - TrainerSpell structure - Spell data with requirements/costs
 */
class TrainerInteractionManager
{
public:
    /**
     * @brief Training priority levels
     *
     * Determines which spells are prioritized:
     * - ESSENTIAL: Core class abilities that must be learned
     * - HIGH: Important combat spells and abilities
     * - MEDIUM: Utility spells and situational abilities
     * - LOW: Convenience spells, mounts, professions
     */
    enum class TrainingPriority : uint8
    {
        ESSENTIAL = 0,  // Core rotation abilities
        HIGH      = 1,  // Combat utilities (interrupts, defensives)
        MEDIUM    = 2,  // Utility spells
        LOW       = 3   // Convenience, mounts
    };

    /**
     * @brief Spell evaluation result
     */
    struct SpellEvaluation
    {
        uint32 spellId;               // Spell to learn
        TrainingPriority priority;     // Priority level
        uint32 cost;                   // Training cost in copper
        uint32 requiredLevel;          // Required level to learn
        uint32 requiredSkill;          // Required skill ID (0 for none)
        uint32 requiredSkillValue;     // Required skill level
        bool canLearn;                 // Whether bot can currently learn this
        bool alreadyKnown;             // Whether bot already knows this spell
        ::std::string reason;            // Human-readable reason for priority

        SpellEvaluation()
            : spellId(0), priority(TrainingPriority::LOW), cost(0)
            , requiredLevel(0), requiredSkill(0), requiredSkillValue(0)
            , canLearn(false), alreadyKnown(false)
        { }
    };

    /**
     * @brief Budget allocation for training
     */
    struct TrainingBudget
    {
        uint64 totalAvailable;       // Total gold available
        uint64 reservedForEssential; // Reserved for essential spells
        uint64 reservedForCombat;    // Reserved for combat utilities
        uint64 reservedForUtility;   // Reserved for utility spells
        uint64 discretionary;        // Optional/convenience spending

        TrainingBudget()
            : totalAvailable(0), reservedForEssential(0)
            , reservedForCombat(0), reservedForUtility(0), discretionary(0)
        { }
    };

    TrainerInteractionManager(Player* bot);
    ~TrainerInteractionManager() = default;

    // Core Training Methods

    /**
     * @brief Learn a specific spell from a trainer
     * @param trainer Trainer creature
     * @param spellId Spell to learn
     * @return True if successfully learned
     *
     * Validates spell availability, level requirements, costs,
     * and executes learning via TrinityCore API.
     */
    bool LearnSpell(Creature* trainer, uint32 spellId);

    /**
     * @brief Learn all available spells from a trainer
     * @param trainer Trainer creature
     * @return Number of spells learned
     *
     * Evaluates all available spells, prioritizes by importance,
     * respects budget constraints, and learns in priority order.
     */
    uint32 LearnAllAvailableSpells(Creature* trainer);

    /**
     * @brief Smart training - automatically determine what to learn
     * @param trainer Trainer creature
     * @return Number of spells learned
     *
     * Analyzes trainer offerings, determines bot needs (based on class/spec),
     * creates optimal training plan within budget, and executes training.
     */
    uint32 SmartTrain(Creature* trainer);

    /**
     * @brief Learn profession from a trainer
     * @param trainer Profession trainer
     * @param professionId Profession skill ID
     * @return True if successfully learned
     */
    bool LearnProfession(Creature* trainer, uint32 professionId);

    /**
     * @brief Unlearn a profession (for respec)
     * @param trainer Profession trainer
     * @param professionId Profession skill ID
     * @return True if successfully unlearned
     */
    bool UnlearnProfession(Creature* trainer, uint32 professionId);

    // Trainer Analysis Methods

    /**
     * @brief Get trainer's available spells
     * @param trainer Trainer creature
     * @return Vector of trainer spells
     */
    ::std::vector<TrainerSpell const*> GetTrainerSpells(Creature* trainer) const;

    /**
     * @brief Evaluate a trainer spell for learning
     * @param trainer Trainer creature
     * @param trainerSpell Spell to evaluate
     * @return Evaluation result with priority and reasoning
     */
    SpellEvaluation EvaluateTrainerSpell(Creature* trainer, TrainerSpell const* trainerSpell) const;

    /**
     * @brief Calculate spell priority based on class and spec
     * @param spellId Spell ID
     * @return Priority level
     */
    TrainingPriority CalculateSpellPriority(uint32 spellId) const;

    /**
     * @brief Check if bot can learn a spell
     * @param trainer Trainer creature
     * @param spellId Spell ID
     * @return True if bot meets all requirements
     */
    bool CanLearnSpell(Creature* trainer, uint32 spellId) const;

    /**
     * @brief Get total cost to train all available spells
     * @param trainer Trainer creature
     * @return Total cost in copper
     */
    uint32 GetTotalTrainingCost(Creature* trainer) const;

    /**
     * @brief Check if bot can afford training
     * @param cost Training cost in copper
     * @return True if bot has sufficient gold
     */
    bool CanAffordTraining(uint32 cost) const;

    // Budget Management

    /**
     * @brief Calculate available budget for training
     * @return Budget allocation structure
     */
    TrainingBudget CalculateBudget() const;

    /**
     * @brief Check if training fits within budget
     * @param cost Spell cost
     * @param priority Spell priority
     * @param budget Current budget allocation
     * @return True if within budget
     */
    bool FitsWithinBudget(uint32 cost, TrainingPriority priority, TrainingBudget const& budget) const;

    // Specialty Methods

    /**
     * @brief Get essential spells for bot's class/spec
     * @return Vector of essential spell IDs
     *
     * Returns the core rotation spells that should always be learned:
     * - Warrior: Mortal Strike, Bloodthirst, Shield Slam
     * - Mage: Fireball, Frostbolt, Arcane Blast
     * - etc.
     */
    ::std::vector<uint32> GetEssentialSpells() const;

    /**
     * @brief Get missing essential spells
     * @return Vector of essential spell IDs the bot doesn't know
     */
    ::std::vector<uint32> GetMissingEssentialSpells() const;

    /**
     * @brief Check if trainer can teach class spells
     * @param trainer Trainer creature
     * @return True if class trainer
     */
    bool IsClassTrainer(Creature* trainer) const;

    /**
     * @brief Check if trainer teaches professions
     * @param trainer Trainer creature
     * @return True if profession trainer
     */
    bool IsProfessionTrainer(Creature* trainer) const;

    /**
     * @brief Get trainer's profession (if profession trainer)
     * @param trainer Trainer creature
     * @return Profession skill ID, or 0 if not profession trainer
     */
    uint32 GetTrainerProfession(Creature* trainer) const;

    // Statistics and Performance

    struct Statistics
    {
        uint32 spellsLearned;       // Total spells learned
        uint64 totalGoldSpent;      // Total gold spent on training
        uint32 trainingSessions;    // Number of training sessions
        uint32 trainingFailures;    // Failed training attempts
        uint32 insufficientGold;    // Failed due to insufficient gold
        uint32 levelTooLow;         // Failed due to level requirements

        Statistics()
            : spellsLearned(0), totalGoldSpent(0), trainingSessions(0)
            , trainingFailures(0), insufficientGold(0), levelTooLow(0)
        { }
    };

    Statistics const& GetStatistics() const { return m_stats; }
    void ResetStatistics() { m_stats = Statistics(); }

    float GetCPUUsage() const { return m_cpuUsage; }
    size_t GetMemoryUsage() const;

private:
    // Internal Helper Methods

    /**
     * @brief Check if spell is an essential class ability
     * @param spellId Spell ID
     * @return True if essential
     */
    bool IsEssentialSpell(uint32 spellId) const;

    /**
     * @brief Check if spell is a combat utility (interrupt, defensive)
     * @param spellId Spell ID
     * @return True if combat utility
     */
    bool IsCombatUtility(uint32 spellId) const;

    /**
     * @brief Execute spell learning via TrinityCore API
     * @param trainer Trainer creature
     * @param spellId Spell to learn
     * @param cost Training cost
     * @return True if successfully learned
     */
    bool ExecuteTraining(Creature* trainer, uint32 spellId, uint32 cost);

    /**
     * @brief Record training in statistics
     */
    void RecordTraining(uint32 spellId, uint32 cost, bool success);

    /**
     * @brief Initialize essential spells cache for bot's class
     */
    void InitializeEssentialSpellsCache();

    // Member Variables
    Player* m_bot;
    Statistics m_stats;

    // Performance Tracking
    float m_cpuUsage;
    uint32 m_totalTrainingTime;   // microseconds
    uint32 m_trainingDecisionCount;

    // Cache for frequently accessed data
    ::std::unordered_map<uint32, TrainingPriority> m_priorityCache;
    ::std::unordered_set<uint32> m_essentialSpellsCache;
    bool m_essentialCacheInitialized;
};

} // namespace Playerbot
