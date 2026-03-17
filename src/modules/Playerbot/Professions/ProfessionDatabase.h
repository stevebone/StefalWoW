/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * PROFESSION DATABASE - Shared Profession Data Repository
 *
 * Phase 1B: ProfessionManager Per-Bot Instance Refactoring (2025-11-18)
 *
 * This singleton manages SHARED profession data that is common to all bots:
 * - Recipe database (all WoW recipes)
 * - Class profession recommendations
 * - Beneficial profession pairs
 * - Race profession bonuses
 * - Profession-specific configurations
 *
 * Architecture:
 * - Singleton pattern (global shared data)
 * - Initialized once at server startup
 * - Thread-safe read access for all bots
 * - Replaces shared data previously in ProfessionManager singleton
 *
 * Rationale:
 * Recipe data, class recommendations, and race bonuses are world constants
 * that don't change per-bot. Keeping them in a singleton is efficient and
 * correct. Per-bot ProfessionManager instances query this shared database.
 */

#pragma once

#ifndef PLAYERBOT_PROFESSION_DATABASE_H
#define PLAYERBOT_PROFESSION_DATABASE_H

#include "Define.h"
#include "Threading/LockHierarchy.h"
#include "Player.h"
#include "SharedDefines.h"
#include <unordered_map>
#include <vector>
#include <mutex>

namespace Playerbot
{

// Forward declarations (defined in ProfessionManager.h)
enum class ProfessionType : uint16;
enum class ProfessionCategory : uint8;
struct RecipeInfo;

/**
 * @brief Shared profession database for all bots
 *
 * **Thread Safety**: All public methods are const and thread-safe for reads.
 * Initialization must complete before any bot accesses the database.
 *
 * **Lifecycle**:
 * 1. Server startup: `ProfessionDatabase::instance()->Initialize()`
 * 2. Bot creation: Per-bot ProfessionManager queries this database
 * 3. Server shutdown: Automatic cleanup via singleton destructor
 *
 * **Design Pattern**: Singleton (Meyer's singleton)
 */
class TC_GAME_API ProfessionDatabase final
{
public:
    static ProfessionDatabase* instance();

    /**
     * @brief Initialize profession database on server startup
     * Loads all recipes, class recommendations, profession pairs, race bonuses
     */
    void Initialize();

    // ========================================================================
    // RECIPE DATABASE QUERIES
    // ========================================================================

    /**
     * @brief Get recipe by ID
     * @return Recipe info, or nullptr if not found
     */
    RecipeInfo const* GetRecipe(uint32 recipeId) const;

    /**
     * @brief Get all recipe IDs for a profession
     * @return Vector of recipe IDs (empty if profession has no recipes)
     */
    std::vector<uint32> GetRecipesForProfession(ProfessionType profession) const;

    /**
     * @brief Get total number of recipes in database
     */
    uint32 GetTotalRecipeCount() const;

    // ========================================================================
    // CLASS PROFESSION RECOMMENDATIONS
    // ========================================================================

    /**
     * @brief Get recommended professions for a class
     * @param classId Class ID (CLASS_WARRIOR, CLASS_PALADIN, etc.)
     * @return Vector of recommended professions (empty if no recommendations)
     */
    std::vector<ProfessionType> GetRecommendedProfessions(uint8 classId) const;

    /**
     * @brief Check if profession is suitable for class
     * @param classId Class ID
     * @param profession Profession to check
     * @return True if profession is in recommended list for class
     */
    bool IsProfessionSuitableForClass(uint8 classId, ProfessionType profession) const;

    // ========================================================================
    // PROFESSION SYNERGY
    // ========================================================================

    /**
     * @brief Get beneficial profession pairs for a profession
     * @param profession Base profession
     * @return Vector of synergistic professions (e.g., Mining -> Blacksmithing)
     */
    std::vector<ProfessionType> GetBeneficialPairs(ProfessionType profession) const;

    /**
     * @brief Check if two professions form a beneficial pair
     * @param prof1 First profession
     * @param prof2 Second profession
     * @return True if they synergize (bidirectional check)
     */
    bool IsBeneficialPair(ProfessionType prof1, ProfessionType prof2) const;

    // ========================================================================
    // RACE PROFESSION BONUSES
    // ========================================================================

    /**
     * @brief Get race-specific profession skill bonus
     * @param raceId Race ID (RACE_HUMAN, RACE_TAUREN, etc.)
     * @param profession Profession to check
     * @return Skill bonus amount (0 if no bonus)
     *
     * Examples:
     * - Tauren: +15 Herbalism
     * - Blood Elf: +10 Enchanting
     * - Dwarf: +10 Mining
     */
    uint16 GetRaceProfessionBonus(uint8 raceId, ProfessionType profession) const;

    // ========================================================================
    // PROFESSION METADATA
    // ========================================================================

    /**
     * @brief Get profession category
     * @param profession Profession to categorize
     * @return PRODUCTION, GATHERING, or SECONDARY
     */
    ProfessionCategory GetProfessionCategory(ProfessionType profession) const;

private:
    ProfessionDatabase();
    ~ProfessionDatabase() = default;

    // ========================================================================
    // INITIALIZATION HELPERS
    // ========================================================================

    void LoadRecipeDatabase();
    void LoadProfessionRecommendations();
    void InitializeClassProfessions();
    void InitializeProfessionPairs();
    void InitializeRaceBonuses();

    // Per-class initialization
    void InitializeWarriorProfessions();
    void InitializePaladinProfessions();
    void InitializeHunterProfessions();
    void InitializeRogueProfessions();
    void InitializePriestProfessions();
    void InitializeShamanProfessions();
    void InitializeMageProfessions();
    void InitializeWarlockProfessions();
    void InitializeDruidProfessions();
    void InitializeDeathKnightProfessions();
    void InitializeMonkProfessions();
    void InitializeDemonHunterProfessions();
    void InitializeEvokerProfessions();

    // ========================================================================
    // SHARED DATA STRUCTURES
    // ========================================================================

    // Recipe database (recipeId -> RecipeInfo)
    std::unordered_map<uint32, RecipeInfo> _recipes;

    // Profession recipes (profession -> vector of recipeIds)
    std::unordered_map<ProfessionType, std::vector<uint32>> _professionRecipes;

    // Class profession recommendations (classId -> preferred professions)
    std::unordered_map<uint8, std::vector<ProfessionType>> _classRecommendations;

    // Beneficial profession pairs (profession -> synergistic partners)
    std::unordered_map<ProfessionType, std::vector<ProfessionType>> _professionPairs;

    // Race profession bonuses (raceId -> (profession -> bonus))
    std::unordered_map<uint8, std::unordered_map<ProfessionType, uint16>> _raceBonuses;

    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::PROFESSION_MANAGER> _mutex;

    // Non-copyable
    ProfessionDatabase(ProfessionDatabase const&) = delete;
    ProfessionDatabase& operator=(ProfessionDatabase const&) = delete;
};

} // namespace Playerbot

#endif // PLAYERBOT_PROFESSION_DATABASE_H
