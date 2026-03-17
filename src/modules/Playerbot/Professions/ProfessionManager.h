/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * COMPLETE PROFESSION MANAGEMENT SYSTEM FOR PLAYERBOT
 *
 * Phase 1B Refactoring (2025-11-18):
 * - Converted from global singleton to per-bot instance
 * - Shared data moved to ProfessionDatabase singleton
 * - Per-bot data now direct members (not maps with playerGuid keys)
 *
 * This system provides comprehensive profession automation for all 14 WoW professions:
 * - Auto-learn professions based on class
 * - Auto-level professions (1-450 skill)
 * - Crafting automation with optimal recipe selection
 * - Gathering automation (Mining, Herbalism, Skinning)
 * - Recipe learning and discovery
 * - Integration with TradeAutomation and EquipmentManager
 *
 * Architecture:
 * - ProfessionManager: Per-bot instance managing bot's profession operations
 * - ProfessionDatabase: Shared singleton for recipe/recommendation data
 * - CraftingAutomation: Automated crafting queues and material management
 * - GatheringAutomation: Resource node detection and harvesting
 */

#pragma once

#include "Define.h"
#include "Threading/LockHierarchy.h"
#include "Player.h"
#include "ObjectGuid.h"
#include "SharedDefines.h"
#include <unordered_set>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>

namespace Playerbot
{

/**
 * @brief Profession types matching WoW 12.0 skill IDs
 */
enum class ProfessionType : uint16
{
    NONE = 0,

    // Primary Professions (Production)
    ALCHEMY = SKILL_ALCHEMY,                    // 171
    BLACKSMITHING = SKILL_BLACKSMITHING,        // 164
    ENCHANTING = SKILL_ENCHANTING,              // 333
    ENGINEERING = SKILL_ENGINEERING,            // 202
    INSCRIPTION = SKILL_INSCRIPTION,            // 773
    JEWELCRAFTING = SKILL_JEWELCRAFTING,        // 755
    LEATHERWORKING = SKILL_LEATHERWORKING,      // 165
    TAILORING = SKILL_TAILORING,                // 197

    // Primary Professions (Gathering)
    MINING = SKILL_MINING,                      // 186
    HERBALISM = SKILL_HERBALISM,                // 182
    SKINNING = SKILL_SKINNING,                  // 393

    // Secondary Professions
    COOKING = SKILL_COOKING,                    // 185
    FISHING = SKILL_FISHING,                    // 356
    FIRST_AID = 129                             // First Aid (if still in 12.0)
};

/**
 * @brief Profession categories for automation logic
 */
enum class ProfessionCategory : uint8
{
    PRODUCTION,     // Alchemy, Blacksmithing, Enchanting, etc.
    GATHERING,      // Mining, Herbalism, Skinning
    SECONDARY       // Cooking, Fishing, First Aid
};

/**
 * @brief Recipe information for crafting
 */
struct RecipeInfo
{
    uint32 recipeId;
    uint32 spellId;
    ProfessionType profession;
    uint16 requiredSkill;
    uint16 skillUpOrange;    // Always skill up
    uint16 skillUpYellow;    // Often skill up
    uint16 skillUpGreen;     // Rarely skill up
    uint16 skillUpGray;      // Never skill up

    struct Reagent
    {
        uint32 itemId;
        uint32 quantity;
    };
    std::vector<Reagent> reagents;

    uint32 productItemId;
    uint32 productQuantity;
    uint32 createdItemType;  // For disenchant/prospect/mill products

    bool isDiscovery;        // Learned from discovery
    bool isTrainer;          // Learned from trainer
    bool isWorldDrop;        // Learned from world drop

    RecipeInfo() : recipeId(0), spellId(0), profession(ProfessionType::NONE),
        requiredSkill(0), skillUpOrange(0), skillUpYellow(0), skillUpGreen(0), skillUpGray(0),
        productItemId(0), productQuantity(1), createdItemType(0),
        isDiscovery(false), isTrainer(true), isWorldDrop(false) {}
};

/**
 * @brief Profession skill tracking
 */
struct ProfessionSkillInfo
{
    ProfessionType profession;
    uint16 currentSkill;
    uint16 maxSkill;
    uint32 lastUpdate;
    bool isPrimary;          // Primary vs secondary profession

    ProfessionSkillInfo() : profession(ProfessionType::NONE), currentSkill(0),
        maxSkill(0), lastUpdate(0), isPrimary(true) {}
};

/**
 * @brief Profession automation configuration per bot
 */
struct ProfessionAutomationProfile
{
    bool autoLearnProfessions = true;      // Auto-learn professions based on class
    bool autoLevelProfessions = true;      // Auto-level through crafting
    bool preferGathering = false;          // Prioritize gathering over crafting
    bool craftForSelf = true;              // Craft gear/consumables for self
    bool craftForProfit = true;            // Craft items to sell
    float skillUpThreshold = 0.8f;         // Min skill-up chance (0.0-1.0)
    uint32 maxCraftingBudget = 50000;      // Max gold to spend on materials (copper)
    bool learnAllRecipes = false;          // Learn all available recipes vs optimal only

    // Class-specific profession preferences
    std::vector<ProfessionType> preferredProfessions;
    std::unordered_set<uint32> neverCraftItems;

    ProfessionAutomationProfile() = default;
};

/**
 * @brief Per-bot profession manager
 *
 * **Phase 1B Refactoring (2025-11-18)**:
 * - Converted from global singleton to per-bot instance
 * - Owned by GameSystemsManager (one instance per bot)
 * - Queries ProfessionDatabase for shared data (recipes, recommendations)
 * - Stores per-bot data directly (not in maps with playerGuid keys)
 *
 * **Thread Safety**:
 * - Each bot has its own instance (no sharing between bots)
 * - Internal mutex protects bot's profession data
 * - ProfessionDatabase queries are thread-safe
 *
 * **Lifecycle**:
 * 1. Created by GameSystemsManager when bot spawns
 * 2. Destroyed when bot despawns (RAII via unique_ptr)
 */

/**
 * @brief Profession statistics tracking
 */
struct ProfessionMetrics
{
    ::std::atomic<uint32> professionsLearned;
    ::std::atomic<uint32> recipesLearned;
    ::std::atomic<uint32> itemsCrafted;
    ::std::atomic<uint32> resourcesGathered;
    ::std::atomic<uint32> skillPointsGained;
    ::std::atomic<uint32> goldSpentOnMaterials;
    ::std::atomic<uint32> goldEarnedFromCrafts;

    void Reset()
    {
        professionsLearned = 0;
        recipesLearned = 0;
        itemsCrafted = 0;
        resourcesGathered = 0;
        skillPointsGained = 0;
        goldSpentOnMaterials = 0;
        goldEarnedFromCrafts = 0;
    }
};

class TC_GAME_API ProfessionManager final
{
public:
    /**
     * @brief Constructor for per-bot instance
     * @param bot Non-owning pointer to bot player
     */
    explicit ProfessionManager(Player* bot);
    ~ProfessionManager();

    // ============================================================================
    // CORE PROFESSION MANAGEMENT
    // ============================================================================

    /**
     * Initialize profession system on server startup
     * NOTE: This is now a no-op. Use ProfessionDatabase::instance()->Initialize() instead.
     */
    void Initialize();

    /**
     * Update profession automation for this bot (called periodically)
     */
    void Update(uint32 diff);

    /**
     * Learn profession for this bot
     */
    bool LearnProfession(ProfessionType profession);

    /**
     * Check if this bot has profession
     */
    bool HasProfession(ProfessionType profession) const;

    /**
     * Get profession skill level for this bot
     */
    uint16 GetProfessionSkill(ProfessionType profession) const;

    /**
     * Get max profession skill for this bot
     */
    uint16 GetMaxProfessionSkill(ProfessionType profession) const;

    /**
     * Get all professions for this bot
     */
    std::vector<ProfessionSkillInfo> GetPlayerProfessions() const;

    /**
     * Unlearn profession (for respec)
     */
    bool UnlearnProfession(ProfessionType profession);

    // ============================================================================
    // AUTO-LEARN SYSTEM
    // ============================================================================

    /**
     * Auto-learn professions based on class
     * Called when bot is created or profession slots are empty
     */
    void AutoLearnProfessionsForClass();

    /**
     * Get recommended professions for class
     * Queries ProfessionDatabase
     */
    std::vector<ProfessionType> GetRecommendedProfessions(uint8 classId) const;

    /**
     * Check if profession is suitable for class
     * Queries ProfessionDatabase
     */
    bool IsProfessionSuitableForClass(uint8 classId, ProfessionType profession) const;

    /**
     * Get profession category
     * Queries ProfessionDatabase
     */
    ProfessionCategory GetProfessionCategory(ProfessionType profession) const;

    /**
     * Get beneficial profession pair for a given profession
     * Example: Mining → Blacksmithing/Engineering/Jewelcrafting
     * Queries ProfessionDatabase
     */
    std::vector<ProfessionType> GetBeneficialPairs(ProfessionType profession) const;

    /**
     * Check if two professions form a beneficial pair
     * Queries ProfessionDatabase
     */
    bool IsBeneficialPair(ProfessionType prof1, ProfessionType prof2) const;

    /**
     * Get race-specific profession skill bonus
     * Example: Tauren +15 Herbalism, Blood Elf +10 Enchanting
     * Queries ProfessionDatabase
     */
    uint16 GetRaceProfessionBonus(uint8 raceId, ProfessionType profession) const;

    // ============================================================================
    // RECIPE MANAGEMENT
    // ============================================================================

    /**
     * Learn recipe for this bot
     */
    bool LearnRecipe(uint32 recipeId);

    /**
     * Check if this bot knows recipe
     */
    bool KnowsRecipe(uint32 recipeId) const;

    /**
     * Get all recipes for profession
     * Queries ProfessionDatabase
     */
    std::vector<RecipeInfo> GetRecipesForProfession(ProfessionType profession) const;

    /**
     * Get craftable recipes for this bot (has skill + reagents)
     */
    std::vector<RecipeInfo> GetCraftableRecipes(ProfessionType profession) const;

    /**
     * Get optimal recipe for leveling (highest skill-up chance)
     */
    RecipeInfo const* GetOptimalLevelingRecipe(ProfessionType profession) const;

    /**
     * Check if this bot can craft recipe
     */
    bool CanCraftRecipe(RecipeInfo const& recipe) const;

    /**
     * Get skill-up probability for recipe (0.0-1.0)
     */
    float GetSkillUpChance(RecipeInfo const& recipe) const;

    // ============================================================================
    // CRAFTING AUTOMATION
    // ============================================================================

    /**
     * Auto-level profession by crafting optimal recipes
     */
    bool AutoLevelProfession(ProfessionType profession);

    /**
     * Craft item (single craft)
     */
    bool CraftItem(RecipeInfo const& recipe, uint32 quantity = 1);

    /**
     * Queue crafting task for automation
     */
    void QueueCraft(uint32 recipeId, uint32 quantity);

    /**
     * Process crafting queue (called in Update)
     */
    void ProcessCraftingQueue(uint32 diff);

    /**
     * Check if this bot has materials for recipe
     */
    bool HasMaterialsForRecipe(RecipeInfo const& recipe) const;

    /**
     * Get missing materials for recipe
     */
    std::vector<std::pair<uint32, uint32>> GetMissingMaterials(RecipeInfo const& recipe) const;

    // ============================================================================
    // AUTOMATION PROFILES
    // ============================================================================

    void SetAutomationProfile(ProfessionAutomationProfile const& profile);
    ProfessionAutomationProfile GetAutomationProfile() const;

    // ============================================================================
    // METRICS
    // ============================================================================

    ProfessionMetrics const& GetMetrics() const;
    ProfessionMetrics const& GetGlobalMetrics() const;

private:
    // ============================================================================
    // CRAFTING HELPERS
    // ============================================================================

    bool CastCraftingSpell(RecipeInfo const& recipe);
    bool ConsumeMaterials(RecipeInfo const& recipe);
    void HandleCraftingResult(RecipeInfo const& recipe, bool success);

    // ============================================================================
    // SKILL CALCULATION HELPERS
    // ============================================================================

    uint16 CalculateSkillUpAmount(RecipeInfo const& recipe, uint16 currentSkill) const;
    bool ShouldCraftForSkillUp(RecipeInfo const& recipe) const;

    // ============================================================================
    // PER-BOT DATA MEMBERS
    // ============================================================================

    Player* _bot;  // Non-owning pointer to bot

    // Automation profile for this bot
    ProfessionAutomationProfile _profile;

    // Crafting queue for this bot
    struct CraftingTask
    {
        uint32 recipeId;
        uint32 quantity;
        uint32 queueTime;
    };
    std::vector<CraftingTask> _craftingQueue;

    // Metrics for this bot
    ProfessionMetrics _metrics;
    static ProfessionMetrics _globalMetrics;  // Shared across all bots

    // Update timing
    uint32 _lastUpdateTime{0};

    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::PROFESSION_MANAGER> _mutex;

    // Update intervals
    static constexpr uint32 PROFESSION_UPDATE_INTERVAL = 5000;  // 5 seconds
    static constexpr uint32 CRAFTING_CAST_TIME = 3000;          // 3 seconds per craft

    // Non-copyable, non-movable
    ProfessionManager(ProfessionManager const&) = delete;
    ProfessionManager& operator=(ProfessionManager const&) = delete;
    ProfessionManager(ProfessionManager&&) = delete;
    ProfessionManager& operator=(ProfessionManager&&) = delete;
};

} // namespace Playerbot
