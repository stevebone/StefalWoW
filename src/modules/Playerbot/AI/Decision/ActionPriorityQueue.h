/*
 * Copyright (C) 2024+ TrinityCore <http://www.trinitycore.org/>
 *
 * Phase 5 Enhancement: Action Priority Queue
 *
 * This system provides intelligent spell priority management for combat rotations.
 * It dynamically adjusts spell priorities based on cooldowns, resources, and
 * combat situations, then provides recommendations to the DecisionFusion system.
 */

#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include "../Common/ActionScoringEngine.h"  // For CombatContext enum
#include <vector>
#include <queue>
#include <unordered_map>
#include <functional>

class Player;
class Unit;

namespace Playerbot {
namespace bot { namespace ai {

// Forward declarations
struct DecisionVote;

/**
 * @enum SpellPriority
 * @brief Priority levels for spell execution
 */
enum class SpellPriority : uint8
{
    EMERGENCY = 100,    // Life-saving abilities (Divine Shield, Ice Block)
    CRITICAL = 90,      // Critical cooldowns (Bloodlust, Guardian Spirit)
    HIGH = 70,          // Core rotation abilities (Fireball, Mortal Strike)
    MEDIUM = 50,        // Situational abilities (AoE, CC)
    LOW = 30,           // Filler abilities (Frostbolt, Auto-attack)
    OPTIONAL_PRIORITY = 10       // Optional abilities (buffs outside combat)
};

/**
 * @enum SpellCategory
 * @brief Categories for spell classification
 */
enum class SpellCategory : uint8
{
    DEFENSIVE,          // Defensive cooldowns
    OFFENSIVE,          // Offensive cooldowns
    HEALING,            // Healing spells
    CROWD_CONTROL,      // CC abilities
    UTILITY,            // Utility spells (buffs, dispels)
    DAMAGE_SINGLE,      // Single-target damage
    DAMAGE_AOE,         // AoE damage
    RESOURCE_BUILDER,   // Resource generators
    RESOURCE_SPENDER,   // Resource spenders
    MOVEMENT            // Movement abilities
};

/**
 * @struct SpellCondition
 * @brief Conditions that must be met for a spell to be usable
 */
struct SpellCondition
{
    ::std::function<bool(Player*, Unit*)> condition; // Condition function
    ::std::string description;                        // Human-readable description

    SpellCondition(::std::function<bool(Player*, Unit*)> cond, const ::std::string& desc)
        : condition(cond), description(desc) {}
};

/**
 * @struct PrioritizedSpell
 * @brief Represents a spell with its priority and conditions
 */
struct PrioritizedSpell
{
    uint32 spellId;                             // Spell ID
    SpellPriority basePriority;                 // Base priority level
    SpellCategory category;                     // Spell category
    ::std::vector<SpellCondition> conditions;     // Conditions for casting
    float priorityMultiplier;                   // Dynamic priority multiplier (1.0 = normal)
    uint32 lastCastTime;                        // Last time this spell was cast

    PrioritizedSpell()
        : spellId(0)
        , basePriority(SpellPriority::MEDIUM)
        , category(SpellCategory::DAMAGE_SINGLE)
        , priorityMultiplier(1.0f)
        , lastCastTime(0)
    {}

    PrioritizedSpell(uint32 id, SpellPriority priority, SpellCategory cat)
        : spellId(id)
        , basePriority(priority)
        , category(cat)
        , priorityMultiplier(1.0f)
        , lastCastTime(0)
    {}

    /**
     * @brief Calculate effective priority for this spell
     * @param bot Bot casting the spell
     * @param target Target of the spell
     * @param context Current combat context
     * @return Effective priority score (0.0-1.0)
     */
    [[nodiscard]] float CalculateEffectivePriority(Player* bot, Unit* target, CombatContext context) const;

    /**
     * @brief Check if all conditions are met
     * @param bot Bot casting the spell
     * @param target Target of the spell
     * @return true if all conditions are met
     */
    [[nodiscard]] bool AreConditionsMet(Player* bot, Unit* target) const;
};

/**
 * @class ActionPriorityQueue
 * @brief Manages spell priority queue for intelligent rotation execution
 *
 * This class provides a dynamic spell priority system that:
 * - Maintains per-class spell priority lists
 * - Adjusts priorities based on combat situations
 * - Checks cooldowns, resources, and conditions
 * - Provides spell recommendations to DecisionFusion
 *
 * **Integration with DecisionFusion**:
 * ActionPriorityQueue generates DecisionVotes for the top-priority available spell,
 * which are then fused with votes from other systems (BehaviorPriority, BehaviorTrees, etc.)
 *
 * **Usage Example**:
 * @code
 * ActionPriorityQueue queue;
 *
 * // Register Fire Mage rotation
 * queue.RegisterSpell(FIREBALL, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);
 * queue.RegisterSpell(PYROBLAST, SpellPriority::CRITICAL, SpellCategory::DAMAGE_SINGLE);
 * queue.AddCondition(PYROBLAST, [](Player* bot, Unit*) {
 *     return bot->HasAura(HOT_STREAK); // Only cast with Hot Streak proc
 * }, "Hot Streak proc");
 *
 * // Get highest priority spell
 * uint32 bestSpell = queue.GetHighestPrioritySpell(bot, target, context);
 * if (bestSpell != 0)
 *     CastSpell(bestSpell, target);
 * @endcode
 *
 * **Performance**:
 * - O(n log n) per query (n = registered spells, typically 10-20)
 * - ~500 bytes per bot
 * - <0.1ms per query
 */
class TC_GAME_API ActionPriorityQueue
{
public:
    ActionPriorityQueue();

    /**
     * @brief Register a spell in the priority queue
     * @param spellId Spell ID
     * @param priority Base priority level
     * @param category Spell category
     */
    void RegisterSpell(uint32 spellId, SpellPriority priority, SpellCategory category);

    /**
     * @brief Add a condition for spell casting
     * @param spellId Spell ID
     * @param condition Condition function
     * @param description Human-readable description
     */
    void AddCondition(uint32 spellId, ::std::function<bool(Player*, Unit*)> condition, const ::std::string& description);

    /**
     * @brief Set priority multiplier for dynamic priority adjustments
     * @param spellId Spell ID
     * @param multiplier Priority multiplier (1.0 = normal, 2.0 = double priority)
     */
    void SetPriorityMultiplier(uint32 spellId, float multiplier);

    /**
     * @brief Get highest priority spell that can be cast
     * @param bot Bot casting the spell
     * @param target Target of the spell
     * @param context Current combat context
     * @return Spell ID of highest priority spell, or 0 if none available
     */
    [[nodiscard]] uint32 GetHighestPrioritySpell(Player* bot, Unit* target, CombatContext context) const;

    /**
     * @brief Get vote for DecisionFusion system
     * @param bot Bot casting the spell
     * @param target Target of the spell
     * @param context Current combat context
     * @return DecisionVote for highest priority spell
     */
    [[nodiscard]] DecisionVote GetVote(Player* bot, Unit* target, CombatContext context) const;

    /**
     * @brief Get all available spells sorted by priority
     * @param bot Bot casting the spell
     * @param target Target of the spell
     * @param context Current combat context
     * @return Vector of spell IDs sorted by priority (highest first)
     */
    [[nodiscard]] ::std::vector<uint32> GetPrioritizedSpells(Player* bot, Unit* target, CombatContext context) const;

    /**
     * @brief Record that a spell was cast (for cooldown tracking)
     * @param spellId Spell ID
     */
    void RecordCast(uint32 spellId);

    /**
     * @brief Clear all registered spells
     */
    void Clear();

    /**
     * @brief Get number of registered spells
     */
    [[nodiscard]] size_t GetSpellCount() const { return _spells.size(); }

    /**
     * @brief Enable/disable debug logging
     */
    void EnableDebugLogging(bool enable) { _debugLogging = enable; }

private:
    // Registered spells
    ::std::vector<PrioritizedSpell> _spells;

    // Debug logging
    bool _debugLogging;

    /**
     * @brief Check if spell is on cooldown
     */
    [[nodiscard]] bool IsOnCooldown(Player* bot, uint32 spellId) const;

    /**
     * @brief Check if bot has enough resources for spell
     */
    [[nodiscard]] bool HasEnoughResources(Player* bot, uint32 spellId) const;

    /**
     * @brief Check if target is valid for spell
     */
    [[nodiscard]] bool IsValidTarget(Player* bot, Unit* target, uint32 spellId) const;

    /**
     * @brief Find spell in registered list
     */
    [[nodiscard]] PrioritizedSpell* FindSpell(uint32 spellId);
    [[nodiscard]] const PrioritizedSpell* FindSpell(uint32 spellId) const;
};

}} // namespace bot::ai
} // namespace Playerbot
