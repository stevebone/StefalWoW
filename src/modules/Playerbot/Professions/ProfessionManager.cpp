/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * Phase 1B Refactoring (2025-11-18):
 * - Converted from global singleton to per-bot instance
 * - Removed Initialize() - now in ProfessionDatabase
 * - All methods now operate on _bot member (no Player* parameters)
 * - Shared data queries delegate to ProfessionDatabase
 */

#include "ProfessionManager.h"
#include "ProfessionDatabase.h"
#include "Player.h"
#include "Item.h"
#include "Bag.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "Log.h"
#include "DB2Stores.h"
#include "SkillDiscovery.h"
#include <algorithm>
#include "GameTime.h"

namespace Playerbot
{

// ============================================================================
// STATIC MEMBERS
// ============================================================================

ProfessionMetrics ProfessionManager::_globalMetrics;

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

ProfessionManager::ProfessionManager(Player* bot)
    : _bot(bot)
    , _lastUpdateTime(0)
{
    if (_bot)
    {
        TC_LOG_DEBUG("playerbot", "ProfessionManager: Creating instance for bot '{}'", _bot->GetName());
    }
}

ProfessionManager::~ProfessionManager()
{
    // CRITICAL: No logging in destructors - can throw std::bad_alloc during memory pressure
    // which causes std::terminate() if called during stack unwinding
}

// ============================================================================
// INITIALIZATION (NOW NO-OP)
// ============================================================================

void ProfessionManager::Initialize()
{
    // No-op: Shared data initialization moved to ProfessionDatabase::instance()->Initialize()
    // This method kept for interface compatibility
}

// ============================================================================
// CORE PROFESSION MANAGEMENT
// ============================================================================

void ProfessionManager::Update(uint32 diff)
{
    if (!_bot)
        return;

    // Throttle updates to PROFESSION_UPDATE_INTERVAL
    uint32 now = GameTime::GetGameTimeMS();
    if (_lastUpdateTime + PROFESSION_UPDATE_INTERVAL > now)
        return;

    _lastUpdateTime = now;

    // Auto-learn professions if none learned
    if (_profile.autoLearnProfessions)
    {
        std::vector<ProfessionSkillInfo> professions = GetPlayerProfessions();
        if (professions.empty())
        {
            AutoLearnProfessionsForClass();
        }
    }

    // Auto-level professions through crafting
    if (_profile.autoLevelProfessions)
    {
        // Process existing queue first
        ProcessCraftingQueue(diff);

        // If queue is empty, try to queue leveling crafts for each profession
        if (_craftingQueue.empty())
        {
            std::vector<ProfessionSkillInfo> professions = GetPlayerProfessions();
            for (ProfessionSkillInfo const& profInfo : professions)
            {
                if (profInfo.profession == ProfessionType::NONE)
                    continue;

                // Check if profession needs leveling (current < max skill for level)
                uint16 maxSkillForLevel = std::min<uint16>(_bot->GetLevel() * 5, 450);
                if (profInfo.currentSkill >= maxSkillForLevel)
                    continue;

                // Find optimal recipe for this profession
                RecipeInfo const* recipe = GetOptimalLevelingRecipe(profInfo.profession);
                if (recipe)
                {
                    // Queue 1 craft - will be processed next update cycle
                    QueueCraft(recipe->recipeId, 1);
                    TC_LOG_DEBUG("playerbots", "ProfessionManager::Update - {} queued {} for leveling {} (skill {}/{})",
                        _bot->GetName(), recipe->recipeId, static_cast<uint32>(profInfo.profession),
                        profInfo.currentSkill, maxSkillForLevel);
                    break; // Only queue one craft per update to avoid spam
                }
            }
        }
    }
}

bool ProfessionManager::LearnProfession(ProfessionType profession)
{
    if (!_bot || profession == ProfessionType::NONE)
        return false;

    uint32 skillId = static_cast<uint32>(profession);

    // Check if already has profession
    if (_bot->HasSkill(skillId))
    {
        TC_LOG_DEBUG("playerbots", "Player {} already has profession {}",
            _bot->GetName(), skillId);
        return false;
    }

    // Check 2-major profession limit (secondary professions are unlimited)
    ProfessionCategory category = GetProfessionCategory(profession);
    if (category == ProfessionCategory::PRODUCTION || category == ProfessionCategory::GATHERING)
    {
        // Count current major professions
        uint32 majorProfessionCount = 0;
        std::vector<ProfessionSkillInfo> currentProfessions = GetPlayerProfessions();
        for (ProfessionSkillInfo const& info : currentProfessions)
        {
            if (info.isPrimary)
                majorProfessionCount++;
        }

        // Enforce 2-major profession limit
        if (majorProfessionCount >= 2)
        {
            TC_LOG_WARN("playerbots", "Player {} already has 2 major professions, cannot learn {}",
                _bot->GetName(), skillId);
            return false;
        }
    }

    // Learn profession (set skill to 1, max based on level)
    uint16 maxSkill = std::min<uint16>(_bot->GetLevel() * 5, 450);
    _bot->SetSkill(skillId, 1, 1, maxSkill);

    TC_LOG_DEBUG("playerbots", "Player {} learned profession {} (max skill: {})",
        _bot->GetName(), skillId, maxSkill);

    // Update metrics
    _metrics.professionsLearned++;
    _globalMetrics.professionsLearned++;

    return true;
}

bool ProfessionManager::HasProfession(ProfessionType profession) const
{
    if (!_bot || profession == ProfessionType::NONE)
        return false;

    return _bot->HasSkill(static_cast<uint32>(profession));
}

uint16 ProfessionManager::GetProfessionSkill(ProfessionType profession) const
{
    if (!_bot || profession == ProfessionType::NONE)
        return 0;

    return _bot->GetSkillValue(static_cast<uint32>(profession));
}

uint16 ProfessionManager::GetMaxProfessionSkill(ProfessionType profession) const
{
    if (!_bot || profession == ProfessionType::NONE)
        return 0;

    return _bot->GetMaxSkillValue(static_cast<uint32>(profession));
}

std::vector<ProfessionSkillInfo> ProfessionManager::GetPlayerProfessions() const
{
    std::vector<ProfessionSkillInfo> professions;

    if (!_bot)
        return professions;

    // Check all profession types
    static const std::vector<ProfessionType> allProfessions = {
        ProfessionType::ALCHEMY,
        ProfessionType::BLACKSMITHING,
        ProfessionType::ENCHANTING,
        ProfessionType::ENGINEERING,
        ProfessionType::INSCRIPTION,
        ProfessionType::JEWELCRAFTING,
        ProfessionType::LEATHERWORKING,
        ProfessionType::TAILORING,
        ProfessionType::MINING,
        ProfessionType::HERBALISM,
        ProfessionType::SKINNING,
        ProfessionType::COOKING,
        ProfessionType::FISHING,
        ProfessionType::FIRST_AID
    };

    for (ProfessionType profession : allProfessions)
    {
        if (HasProfession(profession))
        {
            ProfessionSkillInfo info;
            info.profession = profession;
            info.currentSkill = GetProfessionSkill(profession);
            info.maxSkill = GetMaxProfessionSkill(profession);
            info.lastUpdate = GameTime::GetGameTimeMS();
            info.isPrimary = GetProfessionCategory(profession) != ProfessionCategory::SECONDARY;

            professions.push_back(info);
        }
    }

    return professions;
}

bool ProfessionManager::UnlearnProfession(ProfessionType profession)
{
    if (!_bot || profession == ProfessionType::NONE)
        return false;

    uint32 skillId = static_cast<uint32>(profession);

    if (!_bot->HasSkill(skillId))
        return false;

    // Set skill to 0 to unlearn
    _bot->SetSkill(skillId, 0, 0, 0);

    TC_LOG_DEBUG("playerbots", "Player {} unlearned profession {}",
        _bot->GetName(), skillId);

    return true;
}

// ============================================================================
// AUTO-LEARN SYSTEM
// ============================================================================

void ProfessionManager::AutoLearnProfessionsForClass()
{
    if (!_bot)
        return;

    uint8 classId = _bot->GetClass();
    uint8 raceId = _bot->GetRace();
    std::vector<ProfessionType> recommended = GetRecommendedProfessions(classId);

    if (recommended.empty())
    {
        TC_LOG_WARN("playerbots", "No profession recommendations for class {}", classId);
        return;
    }

    // Try to learn beneficial pairs with racial bonuses considered
    ProfessionType firstProf = ProfessionType::NONE;
    ProfessionType secondProf = ProfessionType::NONE;

    // Priority 1: Check for race-specific bonuses in recommended professions
    for (ProfessionType profession : recommended)
    {
        uint16 raceBonus = GetRaceProfessionBonus(raceId, profession);
        if (raceBonus > 0 && firstProf == ProfessionType::NONE)
        {
            firstProf = profession;
            TC_LOG_INFO("playerbots", "Player {} ({} race) selected {} due to +{} racial bonus",
                _bot->GetName(), static_cast<uint32>(raceId), static_cast<uint32>(profession), raceBonus);
            break;
        }
    }

    // Priority 2: Find beneficial pair for first profession
    if (firstProf != ProfessionType::NONE)
    {
        std::vector<ProfessionType> beneficialPairs = GetBeneficialPairs(firstProf);
        for (ProfessionType pair : beneficialPairs)
        {
            // Check if this pair is in our recommended list
            if (std::find(recommended.begin(), recommended.end(), pair) != recommended.end())
            {
                secondProf = pair;
                TC_LOG_INFO("playerbots", "Player {} selected {} as beneficial pair with {}",
                    _bot->GetName(), static_cast<uint32>(secondProf), static_cast<uint32>(firstProf));
                break;
            }
        }
    }

    // Priority 3: Fall back to first two recommended professions
    if (firstProf == ProfessionType::NONE && !recommended.empty())
    {
        firstProf = recommended[0];
    }

    if (secondProf == ProfessionType::NONE && recommended.size() > 1)
    {
        // Try to find a beneficial pair in remaining recommendations
        for (size_t i = 1; i < recommended.size(); ++i)
        {
            if (IsBeneficialPair(firstProf, recommended[i]))
            {
                secondProf = recommended[i];
                break;
            }
        }

        // If no beneficial pair, just take the second recommendation
        if (secondProf == ProfessionType::NONE)
            secondProf = recommended[1];
    }

    // Learn the selected professions
    if (firstProf != ProfessionType::NONE)
    {
        LearnProfession(firstProf);
    }

    if (secondProf != ProfessionType::NONE)
    {
        LearnProfession(secondProf);
    }

    // Always learn secondary professions (unlimited)
    LearnProfession(ProfessionType::COOKING);
    LearnProfession(ProfessionType::FISHING);
}

std::vector<ProfessionType> ProfessionManager::GetRecommendedProfessions(uint8 classId) const
{
    // Delegate to ProfessionDatabase
    return ProfessionDatabase::instance()->GetRecommendedProfessions(classId);
}

bool ProfessionManager::IsProfessionSuitableForClass(uint8 classId, ProfessionType profession) const
{
    // Delegate to ProfessionDatabase
    return ProfessionDatabase::instance()->IsProfessionSuitableForClass(classId, profession);
}

ProfessionCategory ProfessionManager::GetProfessionCategory(ProfessionType profession) const
{
    // Delegate to ProfessionDatabase
    return ProfessionDatabase::instance()->GetProfessionCategory(profession);
}

std::vector<ProfessionType> ProfessionManager::GetBeneficialPairs(ProfessionType profession) const
{
    // Delegate to ProfessionDatabase
    return ProfessionDatabase::instance()->GetBeneficialPairs(profession);
}

bool ProfessionManager::IsBeneficialPair(ProfessionType prof1, ProfessionType prof2) const
{
    // Delegate to ProfessionDatabase
    return ProfessionDatabase::instance()->IsBeneficialPair(prof1, prof2);
}

uint16 ProfessionManager::GetRaceProfessionBonus(uint8 raceId, ProfessionType profession) const
{
    // Delegate to ProfessionDatabase
    return ProfessionDatabase::instance()->GetRaceProfessionBonus(raceId, profession);
}

// ============================================================================
// RECIPE MANAGEMENT
// ============================================================================

bool ProfessionManager::LearnRecipe(uint32 recipeId)
{
    if (!_bot)
        return false;

    RecipeInfo const* recipe = ProfessionDatabase::instance()->GetRecipe(recipeId);
    if (!recipe)
    {
        TC_LOG_WARN("playerbots", "Unknown recipe ID: {}", recipeId);
        return false;
    }

    // Check if player has the profession
    if (!HasProfession(recipe->profession))
    {
        TC_LOG_DEBUG("playerbots", "Player {} doesn't have profession {} for recipe {}",
            _bot->GetName(), static_cast<uint32>(recipe->profession), recipeId);
        return false;
    }

    // Check skill requirement
    uint16 skill = GetProfessionSkill(recipe->profession);
    if (skill < recipe->requiredSkill)
    {
        TC_LOG_DEBUG("playerbots", "Player {} skill {} too low for recipe {} (requires {})",
            _bot->GetName(), skill, recipeId, recipe->requiredSkill);
        return false;
    }

    // Learn the spell
    _bot->LearnSpell(recipe->spellId, false);

    TC_LOG_DEBUG("playerbots", "Player {} learned recipe {} (spell {})",
        _bot->GetName(), recipeId, recipe->spellId);

    // Update metrics
    _metrics.recipesLearned++;
    _globalMetrics.recipesLearned++;

    return true;
}

bool ProfessionManager::KnowsRecipe(uint32 recipeId) const
{
    if (!_bot)
        return false;

    RecipeInfo const* recipe = ProfessionDatabase::instance()->GetRecipe(recipeId);
    if (!recipe)
        return false;

    return _bot->HasSpell(recipe->spellId);
}

std::vector<RecipeInfo> ProfessionManager::GetRecipesForProfession(ProfessionType profession) const
{
    // Delegate to ProfessionDatabase
    std::vector<uint32> recipeIds = ProfessionDatabase::instance()->GetRecipesForProfession(profession);
    std::vector<RecipeInfo> recipes;

    for (uint32 recipeId : recipeIds)
    {
        RecipeInfo const* recipe = ProfessionDatabase::instance()->GetRecipe(recipeId);
        if (recipe)
            recipes.push_back(*recipe);
    }

    return recipes;
}

std::vector<RecipeInfo> ProfessionManager::GetCraftableRecipes(ProfessionType profession) const
{
    std::vector<RecipeInfo> craftable;

    if (!_bot)
        return craftable;

    std::vector<RecipeInfo> allRecipes = GetRecipesForProfession(profession);

    for (RecipeInfo const& recipe : allRecipes)
    {
        if (CanCraftRecipe(recipe))
            craftable.push_back(recipe);
    }

    return craftable;
}

RecipeInfo const* ProfessionManager::GetOptimalLevelingRecipe(ProfessionType profession) const
{
    if (!_bot)
        return nullptr;

    std::vector<RecipeInfo> craftable = GetCraftableRecipes(profession);

    RecipeInfo const* best = nullptr;
    float bestChance = 0.0f;

    for (RecipeInfo const& recipe : craftable)
    {
        float chance = GetSkillUpChance(recipe);
        if (chance > bestChance)
        {
            bestChance = chance;
            best = &recipe;
        }
    }

    return best;
}

bool ProfessionManager::CanCraftRecipe(RecipeInfo const& recipe) const
{
    if (!_bot)
        return false;

    // Must know the recipe
    if (!_bot->HasSpell(recipe.spellId))
        return false;

    // Must have skill
    uint16 skill = GetProfessionSkill(recipe.profession);
    if (skill < recipe.requiredSkill)
        return false;

    // Must have materials
    return HasMaterialsForRecipe(recipe);
}

float ProfessionManager::GetSkillUpChance(RecipeInfo const& recipe) const
{
    if (!_bot)
        return 0.0f;

    uint16 skill = GetProfessionSkill(recipe.profession);

    // Orange: always skill up
    if (skill < recipe.skillUpOrange)
        return 1.0f;

    // Yellow: often skill up
    if (skill < recipe.skillUpYellow)
        return 0.75f;

    // Green: rarely skill up
    if (skill < recipe.skillUpGreen)
        return 0.25f;

    // Gray: never skill up
    return 0.0f;
}

// ============================================================================
// CRAFTING AUTOMATION
// ============================================================================

bool ProfessionManager::AutoLevelProfession(ProfessionType profession)
{
    if (!_bot)
        return false;

    RecipeInfo const* recipe = GetOptimalLevelingRecipe(profession);
    if (!recipe)
    {
        TC_LOG_DEBUG("playerbots", "No optimal leveling recipe found for profession {}",
            static_cast<uint32>(profession));
        return false;
    }

    return CraftItem(*recipe, 1);
}

bool ProfessionManager::CraftItem(RecipeInfo const& recipe, uint32 quantity)
{
    if (!_bot || quantity == 0)
        return false;

    if (!CanCraftRecipe(recipe))
        return false;

    // Queue the craft
    QueueCraft(recipe.recipeId, quantity);

    return true;
}

void ProfessionManager::QueueCraft(uint32 recipeId, uint32 quantity)
{
    if (!_bot || quantity == 0)
        return;

    CraftingTask task;
    task.recipeId = recipeId;
    task.quantity = quantity;
    task.queueTime = GameTime::GetGameTimeMS();

    _craftingQueue.push_back(task);

    TC_LOG_DEBUG("playerbots", "Queued {} x{} for player {}",
        recipeId, quantity, _bot->GetName());
}

void ProfessionManager::ProcessCraftingQueue(uint32 diff)
{
    if (!_bot)
        return;

    if (_craftingQueue.empty())
        return;

    CraftingTask& task = _craftingQueue.front();

    RecipeInfo const* recipe = ProfessionDatabase::instance()->GetRecipe(task.recipeId);
    if (!recipe)
    {
        // Invalid recipe, remove from queue
        _craftingQueue.erase(_craftingQueue.begin());
        return;
    }

    // Check if we can craft
    if (!CanCraftRecipe(*recipe))
    {
        TC_LOG_DEBUG("playerbots", "Cannot craft recipe {}, removing from queue", task.recipeId);
        _craftingQueue.erase(_craftingQueue.begin());
        return;
    }

    // Craft one item
    if (CastCraftingSpell(*recipe))
    {
        task.quantity--;

        // Update metrics
        _metrics.itemsCrafted++;
        _globalMetrics.itemsCrafted++;

        TC_LOG_DEBUG("playerbots", "Crafted 1x {} ({} remaining in queue)",
            task.recipeId, task.quantity);

        // Remove if finished
        if (task.quantity == 0)
            _craftingQueue.erase(_craftingQueue.begin());
    }
}

bool ProfessionManager::HasMaterialsForRecipe(RecipeInfo const& recipe) const
{
    if (!_bot)
        return false;

    for (RecipeInfo::Reagent const& reagent : recipe.reagents)
    {
        uint32 count = _bot->GetItemCount(reagent.itemId);
        if (count < reagent.quantity)
            return false;
    }

    return true;
}

std::vector<std::pair<uint32, uint32>> ProfessionManager::GetMissingMaterials(RecipeInfo const& recipe) const
{
    std::vector<std::pair<uint32, uint32>> missing;

    if (!_bot)
        return missing;

    for (RecipeInfo::Reagent const& reagent : recipe.reagents)
    {
        uint32 have = _bot->GetItemCount(reagent.itemId);
        if (have < reagent.quantity)
        {
            missing.push_back({reagent.itemId, reagent.quantity - have});
        }
    }

    return missing;
}

// ============================================================================
// CRAFTING HELPERS
// ============================================================================

bool ProfessionManager::CastCraftingSpell(RecipeInfo const& recipe)
{
    if (!_bot)
        return false;

    // Consume materials
    if (!ConsumeMaterials(recipe))
        return false;

    // Cast crafting spell
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(recipe.spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return false;

    // Update profession skill
    _bot->UpdateCraftSkill(spellInfo);

    // Skill point gain
    uint16 oldSkill = GetProfessionSkill(recipe.profession);
    uint16 newSkill = GetProfessionSkill(recipe.profession);
    if (newSkill > oldSkill)
    {
        _metrics.skillPointsGained += (newSkill - oldSkill);
        _globalMetrics.skillPointsGained += (newSkill - oldSkill);
    }

    HandleCraftingResult(recipe, true);

    return true;
}

bool ProfessionManager::ConsumeMaterials(RecipeInfo const& recipe)
{
    if (!_bot)
        return false;

    // Check we have all materials first
    if (!HasMaterialsForRecipe(recipe))
        return false;

    // Destroy reagents
    for (RecipeInfo::Reagent const& reagent : recipe.reagents)
    {
        _bot->DestroyItemCount(reagent.itemId, reagent.quantity, true);
    }

    return true;
}

void ProfessionManager::HandleCraftingResult(RecipeInfo const& recipe, bool success)
{
    if (!_bot || !success)
        return;

    // Item will be created by spell effect, we don't create it manually
    TC_LOG_DEBUG("playerbots", "Player {} successfully crafted item {} from recipe {}",
        _bot->GetName(), recipe.productItemId, recipe.recipeId);
}

// ============================================================================
// SKILL CALCULATION HELPERS
// ============================================================================

uint16 ProfessionManager::CalculateSkillUpAmount(RecipeInfo const& recipe, uint16 currentSkill) const
{
    // Simple skill-up: 1 point per craft
    return 1;
}

bool ProfessionManager::ShouldCraftForSkillUp(RecipeInfo const& recipe) const
{
    if (!_bot)
        return false;

    float chance = GetSkillUpChance(recipe);
    return chance >= _profile.skillUpThreshold;
}

// ============================================================================
// AUTOMATION PROFILES
// ============================================================================

void ProfessionManager::SetAutomationProfile(ProfessionAutomationProfile const& profile)
{
    _profile = profile;
}

ProfessionAutomationProfile ProfessionManager::GetAutomationProfile() const
{
    return _profile;
}

// ============================================================================
// METRICS
// ============================================================================

ProfessionMetrics const& ProfessionManager::GetMetrics() const
{
    return _metrics;
}

ProfessionMetrics const& ProfessionManager::GetGlobalMetrics() const
{
    return _globalMetrics;
}

} // namespace Playerbot
