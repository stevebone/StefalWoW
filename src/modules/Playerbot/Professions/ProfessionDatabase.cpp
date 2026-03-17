/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * Phase 1B: ProfessionManager Per-Bot Instance Refactoring (2025-11-18)
 * 
 * This file contains shared profession data initialization extracted from
 * ProfessionManager singleton. This data is constant across all bots.
 */

#include "ProfessionDatabase.h"
#include "ProfessionManager.h"  // For RecipeInfo, ProfessionType, etc.
#include "Player.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "Log.h"
#include "DB2Stores.h"
#include <algorithm>

namespace Playerbot
{

// ============================================================================
// SINGLETON
// ============================================================================

ProfessionDatabase* ProfessionDatabase::instance()
{
    static ProfessionDatabase instance;
    return &instance;
}

ProfessionDatabase::ProfessionDatabase()
{
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void ProfessionDatabase::Initialize()
{
    std::lock_guard lock(_mutex);

    TC_LOG_INFO("playerbots", "ProfessionDatabase: Initializing shared profession data...");

    LoadRecipeDatabase();
    LoadProfessionRecommendations();
    InitializeProfessionPairs();
    InitializeRaceBonuses();

    TC_LOG_INFO("playerbots", "ProfessionDatabase: Initialized {} recipes, {} profession pairs, {} racial bonuses",
        _recipes.size(), _professionPairs.size(), _raceBonuses.size());
}

// ============================================================================
// RECIPE DATABASE QUERIES
// ============================================================================

RecipeInfo const* ProfessionDatabase::GetRecipe(uint32 recipeId) const
{
    std::lock_guard lock(_mutex);

    auto it = _recipes.find(recipeId);
    if (it != _recipes.end())
        return &it->second;

    return nullptr;
}

std::vector<uint32> ProfessionDatabase::GetRecipesForProfession(ProfessionType profession) const
{
    std::lock_guard lock(_mutex);

    auto it = _professionRecipes.find(profession);
    if (it != _professionRecipes.end())
        return it->second;

    return {};  // Empty vector
}

uint32 ProfessionDatabase::GetTotalRecipeCount() const
{
    std::lock_guard lock(_mutex);
    return static_cast<uint32>(_recipes.size());
}

// ============================================================================
// CLASS PROFESSION RECOMMENDATIONS
// ============================================================================

std::vector<ProfessionType> ProfessionDatabase::GetRecommendedProfessions(uint8 classId) const
{
    std::lock_guard lock(_mutex);

    auto it = _classRecommendations.find(classId);
    if (it != _classRecommendations.end())
        return it->second;

    return {};  // Empty vector
}

bool ProfessionDatabase::IsProfessionSuitableForClass(uint8 classId, ProfessionType profession) const
{
    auto recommended = GetRecommendedProfessions(classId);
    return std::find(recommended.begin(), recommended.end(), profession) != recommended.end();
}

// ============================================================================
// PROFESSION SYNERGY
// ============================================================================

std::vector<ProfessionType> ProfessionDatabase::GetBeneficialPairs(ProfessionType profession) const
{
    std::lock_guard lock(_mutex);

    auto it = _professionPairs.find(profession);
    if (it != _professionPairs.end())
        return it->second;

    return {};  // Empty vector
}

bool ProfessionDatabase::IsBeneficialPair(ProfessionType prof1, ProfessionType prof2) const
{
    auto pairs1 = GetBeneficialPairs(prof1);
    if (std::find(pairs1.begin(), pairs1.end(), prof2) != pairs1.end())
        return true;

    auto pairs2 = GetBeneficialPairs(prof2);
    if (std::find(pairs2.begin(), pairs2.end(), prof1) != pairs2.end())
        return true;

    return false;
}

// ============================================================================
// RACE PROFESSION BONUSES
// ============================================================================

uint16 ProfessionDatabase::GetRaceProfessionBonus(uint8 raceId, ProfessionType profession) const
{
    std::lock_guard lock(_mutex);

    auto raceIt = _raceBonuses.find(raceId);
    if (raceIt != _raceBonuses.end())
    {
        auto profIt = raceIt->second.find(profession);
        if (profIt != raceIt->second.end())
            return profIt->second;
    }

    return 0;  // No bonus
}

// ============================================================================
// PROFESSION METADATA
// ============================================================================

ProfessionCategory ProfessionDatabase::GetProfessionCategory(ProfessionType profession) const
{
    switch (profession)
    {
        case ProfessionType::ALCHEMY:
        case ProfessionType::BLACKSMITHING:
        case ProfessionType::ENCHANTING:
        case ProfessionType::ENGINEERING:
        case ProfessionType::INSCRIPTION:
        case ProfessionType::JEWELCRAFTING:
        case ProfessionType::LEATHERWORKING:
        case ProfessionType::TAILORING:
            return ProfessionCategory::PRODUCTION;

        case ProfessionType::MINING:
        case ProfessionType::HERBALISM:
        case ProfessionType::SKINNING:
            return ProfessionCategory::GATHERING;

        case ProfessionType::COOKING:
        case ProfessionType::FISHING:
        case ProfessionType::FIRST_AID:
            return ProfessionCategory::SECONDARY;

        default:
            return ProfessionCategory::PRODUCTION;  // Default to production
    }
}

// ============================================================================
// INITIALIZATION HELPERS
// ============================================================================

void ProfessionDatabase::LoadRecipeDatabase()
{
    _recipes.clear();
    _professionRecipes.clear();

    uint32 recipeCount = 0;

    // Helper lambda to convert skill ID to profession type
    auto GetProfessionTypeFromSkillId = [](uint16 skillId) -> ProfessionType
    {
        switch (skillId)
        {
            case SKILL_ALCHEMY:         return ProfessionType::ALCHEMY;
            case SKILL_BLACKSMITHING:   return ProfessionType::BLACKSMITHING;
            case SKILL_ENCHANTING:      return ProfessionType::ENCHANTING;
            case SKILL_ENGINEERING:     return ProfessionType::ENGINEERING;
            case SKILL_INSCRIPTION:     return ProfessionType::INSCRIPTION;
            case SKILL_JEWELCRAFTING:   return ProfessionType::JEWELCRAFTING;
            case SKILL_LEATHERWORKING:  return ProfessionType::LEATHERWORKING;
            case SKILL_TAILORING:       return ProfessionType::TAILORING;
            case SKILL_MINING:          return ProfessionType::MINING;
            case SKILL_HERBALISM:       return ProfessionType::HERBALISM;
            case SKILL_SKINNING:        return ProfessionType::SKINNING;
            case SKILL_COOKING:         return ProfessionType::COOKING;
            case SKILL_FISHING:         return ProfessionType::FISHING;
            case 129:                   return ProfessionType::FIRST_AID;  // First Aid
            default:                    return ProfessionType::NONE;
        }
    };

    // Iterate all SkillLineAbility entries from DB2
    for (SkillLineAbilityEntry const* ability : sSkillLineAbilityStore)
    {
        if (!ability)
            continue;

        // Check if this is a profession skill
        ProfessionType profession = GetProfessionTypeFromSkillId(ability->SkillLine);
        if (profession == ProfessionType::NONE)
            continue;

        // Get spell info
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(ability->Spell, DIFFICULTY_NONE);
        if (!spellInfo)
            continue;

        // Create recipe info
        RecipeInfo recipe;
        recipe.spellId = ability->Spell;
        recipe.recipeId = ability->ID;  // Use SkillLineAbility ID as recipe ID
        recipe.profession = profession;
        recipe.requiredSkill = ability->MinSkillLineRank;

        // Calculate skill-up thresholds
        recipe.skillUpOrange = ability->TrivialSkillLineRankHigh;
        recipe.skillUpYellow = (ability->TrivialSkillLineRankHigh + ability->TrivialSkillLineRankLow) / 2;
        recipe.skillUpGreen = ability->TrivialSkillLineRankLow;
        recipe.skillUpGray = (ability->TrivialSkillLineRankLow > 25) ? (ability->TrivialSkillLineRankLow - 25) : 0;

        // Determine acquisition method
        SkillLineAbilityAcquireMethod acquireMethod = ability->GetAcquireMethod();
        if (acquireMethod == SkillLineAbilityAcquireMethod::Learned ||
            acquireMethod == SkillLineAbilityAcquireMethod::AutomaticSkillRank ||
            acquireMethod == SkillLineAbilityAcquireMethod::LearnedOrAutomaticCharLevel)
        {
            recipe.isTrainer = true;
            recipe.isDiscovery = false;
            recipe.isWorldDrop = false;
        }
        else
        {
            recipe.isTrainer = false;
            recipe.isDiscovery = false;
            recipe.isWorldDrop = true;
        }

        // Extract reagents from SpellReagents DB2
        for (SpellReagentsEntry const* reagents : sSpellReagentsStore)
        {
            if (reagents->SpellID == static_cast<int32>(ability->Spell))
            {
                for (uint8 i = 0; i < MAX_SPELL_REAGENTS; ++i)
                {
                    if (reagents->Reagent[i] > 0)
                    {
                        RecipeInfo::Reagent reagent;
                        reagent.itemId = reagents->Reagent[i];
                        reagent.quantity = reagents->ReagentCount[i];
                        recipe.reagents.push_back(reagent);
                    }
                }
                break;  // Found reagents, no need to continue
            }
        }

        // Extract product item from spell effects
        for (SpellEffectInfo const& effect : spellInfo->GetEffects())
        {
            if (effect.Effect == SPELL_EFFECT_CREATE_ITEM)
            {
                recipe.productItemId = effect.ItemType;
                // Try to get quantity from BasePoints (often 1)
                recipe.productQuantity = (effect.BasePoints > 0) ? effect.BasePoints : 1;
                break;
            }
        }

        // Only add if recipe has a product (is actually a crafting spell)
        if (recipe.productItemId > 0)
        {
            _recipes[recipe.recipeId] = recipe;
            _professionRecipes[profession].push_back(recipe.recipeId);
            recipeCount++;
        }
    }

    TC_LOG_INFO("playerbots", "ProfessionDatabase: Loaded {} recipes from SkillLineAbility DB2", recipeCount);
}

void ProfessionDatabase::LoadProfessionRecommendations()
{
    _classRecommendations.clear();

    InitializeWarriorProfessions();
    InitializePaladinProfessions();
    InitializeHunterProfessions();
    InitializeRogueProfessions();
    InitializePriestProfessions();
    InitializeShamanProfessions();
    InitializeMageProfessions();
    InitializeWarlockProfessions();
    InitializeDruidProfessions();
    InitializeDeathKnightProfessions();
    InitializeMonkProfessions();
    InitializeDemonHunterProfessions();
    InitializeEvokerProfessions();

    TC_LOG_DEBUG("playerbots", "ProfessionDatabase: Loaded recommendations for {} classes",
        _classRecommendations.size());
}

void ProfessionDatabase::InitializeClassProfessions()
{
    // Already done in LoadProfessionRecommendations
}

// ============================================================================
// CLASS-SPECIFIC PROFESSION RECOMMENDATIONS
// ============================================================================

void ProfessionDatabase::InitializeWarriorProfessions()
{
    // Warriors wear plate: Blacksmithing + Mining
    // Alternative: Engineering for gadgets
    _classRecommendations[CLASS_WARRIOR] = {
        ProfessionType::BLACKSMITHING,
        ProfessionType::MINING,
        ProfessionType::ENGINEERING
    };
}

void ProfessionDatabase::InitializePaladinProfessions()
{
    // Paladins wear plate: Blacksmithing + Mining
    // Alternative: Jewelcrafting for gems
    _classRecommendations[CLASS_PALADIN] = {
        ProfessionType::BLACKSMITHING,
        ProfessionType::MINING,
        ProfessionType::JEWELCRAFTING
    };
}

void ProfessionDatabase::InitializeHunterProfessions()
{
    // Hunters wear mail: Leatherworking + Skinning
    // Alternative: Engineering for ranged weapons
    _classRecommendations[CLASS_HUNTER] = {
        ProfessionType::LEATHERWORKING,
        ProfessionType::SKINNING,
        ProfessionType::ENGINEERING
    };
}

void ProfessionDatabase::InitializeRogueProfessions()
{
    // Rogues wear leather: Leatherworking + Skinning
    // Alternative: Engineering for gadgets
    _classRecommendations[CLASS_ROGUE] = {
        ProfessionType::LEATHERWORKING,
        ProfessionType::SKINNING,
        ProfessionType::ENGINEERING
    };
}

void ProfessionDatabase::InitializePriestProfessions()
{
    // Priests wear cloth: Tailoring + Enchanting
    // Alternative: Alchemy for potions
    _classRecommendations[CLASS_PRIEST] = {
        ProfessionType::TAILORING,
        ProfessionType::ENCHANTING,
        ProfessionType::ALCHEMY,
        ProfessionType::HERBALISM
    };
}

void ProfessionDatabase::InitializeShamanProfessions()
{
    // Shamans wear mail: Leatherworking + Skinning (or Blacksmithing + Mining)
    // Alternative: Alchemy for potions, Jewelcrafting
    _classRecommendations[CLASS_SHAMAN] = {
        ProfessionType::LEATHERWORKING,
        ProfessionType::SKINNING,
        ProfessionType::ALCHEMY,
        ProfessionType::HERBALISM,
        ProfessionType::JEWELCRAFTING
    };
}

void ProfessionDatabase::InitializeMageProfessions()
{
    // Mages wear cloth: Tailoring + Enchanting
    // Alternative: Alchemy for potions
    _classRecommendations[CLASS_MAGE] = {
        ProfessionType::TAILORING,
        ProfessionType::ENCHANTING,
        ProfessionType::ALCHEMY,
        ProfessionType::HERBALISM
    };
}

void ProfessionDatabase::InitializeWarlockProfessions()
{
    // Warlocks wear cloth: Tailoring + Enchanting
    // Alternative: Alchemy for potions
    _classRecommendations[CLASS_WARLOCK] = {
        ProfessionType::TAILORING,
        ProfessionType::ENCHANTING,
        ProfessionType::ALCHEMY,
        ProfessionType::HERBALISM
    };
}

void ProfessionDatabase::InitializeDruidProfessions()
{
    // Druids wear leather: Leatherworking + Skinning
    // Alternative: Alchemy for potions, Herbalism
    _classRecommendations[CLASS_DRUID] = {
        ProfessionType::LEATHERWORKING,
        ProfessionType::SKINNING,
        ProfessionType::ALCHEMY,
        ProfessionType::HERBALISM
    };
}

void ProfessionDatabase::InitializeDeathKnightProfessions()
{
    // Death Knights wear plate: Blacksmithing + Mining
    // Alternative: Engineering
    _classRecommendations[CLASS_DEATH_KNIGHT] = {
        ProfessionType::BLACKSMITHING,
        ProfessionType::MINING,
        ProfessionType::ENGINEERING
    };
}

void ProfessionDatabase::InitializeMonkProfessions()
{
    // Monks wear leather: Leatherworking + Skinning
    // Alternative: Alchemy for potions
    _classRecommendations[CLASS_MONK] = {
        ProfessionType::LEATHERWORKING,
        ProfessionType::SKINNING,
        ProfessionType::ALCHEMY,
        ProfessionType::HERBALISM
    };
}

void ProfessionDatabase::InitializeDemonHunterProfessions()
{
    // Demon Hunters wear leather: Leatherworking + Skinning
    // Alternative: Engineering
    _classRecommendations[CLASS_DEMON_HUNTER] = {
        ProfessionType::LEATHERWORKING,
        ProfessionType::SKINNING,
        ProfessionType::ENGINEERING
    };
}

void ProfessionDatabase::InitializeEvokerProfessions()
{
    // Evokers wear mail: Leatherworking + Skinning
    // Alternative: Jewelcrafting, Alchemy
    _classRecommendations[CLASS_EVOKER] = {
        ProfessionType::LEATHERWORKING,
        ProfessionType::SKINNING,
        ProfessionType::JEWELCRAFTING,
        ProfessionType::ALCHEMY,
        ProfessionType::HERBALISM
    };
}

// ============================================================================
// PROFESSION PAIRS
// ============================================================================

void ProfessionDatabase::InitializeProfessionPairs()
{
    _professionPairs.clear();

    // GATHERING → PRODUCTION PAIRS

    // Mining pairs
    _professionPairs[ProfessionType::MINING] = {
        ProfessionType::BLACKSMITHING,
        ProfessionType::ENGINEERING,
        ProfessionType::JEWELCRAFTING
    };

    // Herbalism pairs
    _professionPairs[ProfessionType::HERBALISM] = {
        ProfessionType::ALCHEMY,
        ProfessionType::INSCRIPTION
    };

    // Skinning pairs
    _professionPairs[ProfessionType::SKINNING] = {
        ProfessionType::LEATHERWORKING
    };

    // PRODUCTION → GATHERING PAIRS (reciprocal)

    // Blacksmithing pairs
    _professionPairs[ProfessionType::BLACKSMITHING] = {
        ProfessionType::MINING
    };

    // Engineering pairs
    _professionPairs[ProfessionType::ENGINEERING] = {
        ProfessionType::MINING
    };

    // Jewelcrafting pairs
    _professionPairs[ProfessionType::JEWELCRAFTING] = {
        ProfessionType::MINING
    };

    // Alchemy pairs
    _professionPairs[ProfessionType::ALCHEMY] = {
        ProfessionType::HERBALISM
    };

    // Inscription pairs
    _professionPairs[ProfessionType::INSCRIPTION] = {
        ProfessionType::HERBALISM
    };

    // Leatherworking pairs
    _professionPairs[ProfessionType::LEATHERWORKING] = {
        ProfessionType::SKINNING
    };

    // SPECIAL PAIRS

    // Tailoring + Enchanting (cloth gear to disenchant)
    _professionPairs[ProfessionType::TAILORING] = {
        ProfessionType::ENCHANTING
    };

    // Enchanting + Tailoring (mutual benefit)
    _professionPairs[ProfessionType::ENCHANTING] = {
        ProfessionType::TAILORING
    };

    TC_LOG_DEBUG("playerbots", "ProfessionDatabase: Initialized {} beneficial profession pairs",
        _professionPairs.size());
}

// ============================================================================
// RACE BONUSES
// ============================================================================

void ProfessionDatabase::InitializeRaceBonuses()
{
    _raceBonuses.clear();

    // WoW 12.0 Racial Profession Bonuses

    // TAUREN (+15 Herbalism)
    _raceBonuses[RACE_TAUREN][ProfessionType::HERBALISM] = 15;

    // BLOOD ELF (+10 Enchanting)
    _raceBonuses[RACE_BLOODELF][ProfessionType::ENCHANTING] = 10;

    // DRAENEI (+10 Jewelcrafting)
    _raceBonuses[RACE_DRAENEI][ProfessionType::JEWELCRAFTING] = 10;

    // WORGEN (+15 Skinning)
    _raceBonuses[RACE_WORGEN][ProfessionType::SKINNING] = 15;

    // GOBLIN (+15 Alchemy)
    _raceBonuses[RACE_GOBLIN][ProfessionType::ALCHEMY] = 15;

    // PANDAREN (+15 Cooking)
    _raceBonuses[RACE_PANDAREN_NEUTRAL][ProfessionType::COOKING] = 15;
    _raceBonuses[RACE_PANDAREN_ALLIANCE][ProfessionType::COOKING] = 15;
    _raceBonuses[RACE_PANDAREN_HORDE][ProfessionType::COOKING] = 15;

    TC_LOG_DEBUG("playerbots", "ProfessionDatabase: Initialized {} racial profession bonuses",
        _raceBonuses.size());
}

} // namespace Playerbot
