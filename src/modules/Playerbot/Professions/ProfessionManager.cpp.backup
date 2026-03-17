/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "ProfessionManager.h"
#include "Player.h"
#include "Item.h"
#include "Bag.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "Log.h"
#include "DB2Stores.h"
#include "SkillDiscovery.h"
#include <algorithm>

namespace Playerbot
{

// ============================================================================
// SINGLETON
// ============================================================================

ProfessionManager* ProfessionManager::instance()
{
    static ProfessionManager instance;
    return &instance;
}

ProfessionManager::ProfessionManager()
{
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void ProfessionManager::Initialize()
{
    TC_LOG_INFO("playerbots", "ProfessionManager: Initializing profession system...");

    LoadRecipeDatabase();
    LoadProfessionRecommendations();
    InitializeClassProfessions();
    InitializeProfessionPairs();
    InitializeRaceBonuses();

    TC_LOG_INFO("playerbots", "ProfessionManager: Initialized {} recipes, {} profession pairs, {} racial bonuses",
        _recipeDatabase.size(), _professionPairs.size(), _raceBonuses.size());
}

void ProfessionManager::LoadRecipeDatabase()
{
    _recipeDatabase.clear();
    _professionRecipes.clear();

    uint32 recipeCount = 0;

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
            _recipeDatabase[recipe.recipeId] = recipe;
            _professionRecipes[profession].push_back(recipe.recipeId);
            recipeCount++;
        }
    }

    TC_LOG_INFO("playerbots", "ProfessionManager: Loaded {} recipes from SkillLineAbility DB2", recipeCount);
}

// Helper method to convert skill ID to profession type
ProfessionType ProfessionManager::GetProfessionTypeFromSkillId(uint16 skillId) const
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
}

void ProfessionManager::LoadProfessionRecommendations()
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

    TC_LOG_DEBUG("playerbots", "ProfessionManager: Loaded recommendations for {} classes",
        _classRecommendations.size());
}

void ProfessionManager::InitializeClassProfessions()
{
    // Already done in LoadProfessionRecommendations
}

// ============================================================================
// CLASS-SPECIFIC PROFESSION RECOMMENDATIONS
// ============================================================================

void ProfessionManager::InitializeWarriorProfessions()
{
    // Warriors wear plate: Blacksmithing + Mining
    // Alternative: Engineering for gadgets
    _classRecommendations[CLASS_WARRIOR] = {
        ProfessionType::BLACKSMITHING,
        ProfessionType::MINING,
        ProfessionType::ENGINEERING
    };
}

void ProfessionManager::InitializePaladinProfessions()
{
    // Paladins wear plate: Blacksmithing + Mining
    // Alternative: Jewelcrafting for gems
    _classRecommendations[CLASS_PALADIN] = {
        ProfessionType::BLACKSMITHING,
        ProfessionType::MINING,
        ProfessionType::JEWELCRAFTING
    };
}

void ProfessionManager::InitializeHunterProfessions()
{
    // Hunters wear mail: Leatherworking + Skinning
    // Alternative: Engineering for ranged weapons
    _classRecommendations[CLASS_HUNTER] = {
        ProfessionType::LEATHERWORKING,
        ProfessionType::SKINNING,
        ProfessionType::ENGINEERING
    };
}

void ProfessionManager::InitializeRogueProfessions()
{
    // Rogues wear leather: Leatherworking + Skinning
    // Alternative: Engineering for gadgets
    _classRecommendations[CLASS_ROGUE] = {
        ProfessionType::LEATHERWORKING,
        ProfessionType::SKINNING,
        ProfessionType::ENGINEERING
    };
}

void ProfessionManager::InitializePriestProfessions()
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

void ProfessionManager::InitializeShamanProfessions()
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

void ProfessionManager::InitializeMageProfessions()
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

void ProfessionManager::InitializeWarlockProfessions()
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

void ProfessionManager::InitializeDruidProfessions()
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

void ProfessionManager::InitializeDeathKnightProfessions()
{
    // Death Knights wear plate: Blacksmithing + Mining
    // Alternative: Jewelcrafting
    _classRecommendations[CLASS_DEATH_KNIGHT] = {
        ProfessionType::BLACKSMITHING,
        ProfessionType::MINING,
        ProfessionType::JEWELCRAFTING
    };
}

void ProfessionManager::InitializeMonkProfessions()
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

void ProfessionManager::InitializeDemonHunterProfessions()
{
    // Demon Hunters wear leather: Leatherworking + Skinning
    // Alternative: Alchemy
    _classRecommendations[CLASS_DEMON_HUNTER] = {
        ProfessionType::LEATHERWORKING,
        ProfessionType::SKINNING,
        ProfessionType::ALCHEMY,
        ProfessionType::HERBALISM
    };
}

void ProfessionManager::InitializeEvokerProfessions()
{
    // Evokers wear mail: Leatherworking + Skinning (or Blacksmithing + Mining)
    // Alternative: Jewelcrafting, Enchanting
    _classRecommendations[CLASS_EVOKER] = {
        ProfessionType::LEATHERWORKING,
        ProfessionType::SKINNING,
        ProfessionType::JEWELCRAFTING,
        ProfessionType::ENCHANTING
    };
}

// ============================================================================
// CORE PROFESSION MANAGEMENT
// ============================================================================

void ProfessionManager::Update(::Player* player, uint32 diff)
{
    if (!player)
        return;

    uint32 playerGuid = player->GetGUID().GetCounter();
    if (!player)
    {
        TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetGUID");
        return;
    }
    if (!player)
    {
        TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetGUID");
        return;
    }

    // Throttle updates to PROFESSION_UPDATE_INTERVAL
    uint32 now = getMSTime();
    if (_lastUpdateTimes[playerGuid] + PROFESSION_UPDATE_INTERVAL > now)
        return;

    _lastUpdateTimes[playerGuid] = now;

    // Auto-learn professions if none learned
    ProfessionAutomationProfile const& profile = GetAutomationProfile(playerGuid);
    if (profile.autoLearnProfessions)
    {
        std::vector<ProfessionSkillInfo> professions = GetPlayerProfessions(player);
        if (professions.empty())
        {
            AutoLearnProfessionsForClass(player);
        }
    }

    // Process crafting queue
    if (profile.autoLevelProfessions)
    {
        ProcessCraftingQueue(player, diff);
    }
}

bool ProfessionManager::LearnProfession(::Player* player, ProfessionType profession)
{
    if (!player || profession == ProfessionType::NONE)
                if (!player)
                {
                    TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetName");
                    return nullptr;
                }
            if (!player)
            {
                TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetName");
                return;
            }
        return false;

    uint32 skillId = static_cast<uint32>(profession);

    // Check if already has profession
    if (player->HasSkill(skillId))
    {
        TC_LOG_DEBUG("playerbots", "Player {} already has profession {}",
            player->GetName(), skillId);
        return false;
    }

    // Check 2-major profession limit (secondary professions are unlimited)
    ProfessionCategory category = GetProfessionCategory(profession);
    if (category == ProfessionCategory::PRODUCTION || category == ProfessionCategory::GATHERING)
    {
        // Count current major professions
        uint32 majorProfessionCount = 0;
        std::vector<ProfessionSkillInfo> currentProfessions = GetPlayerProfessions(player);
            if (!player)
            {
                TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetName");
                return;
            }
            if (!player)
            {
                TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetLevel");
                return nullptr;
            }
        if (!player)
        {
            TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetName");
            return;
        }
            if (!player)
            {
                TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetGUID");
                return nullptr;
            }
                if (!player)
                {
                    TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetName");
                    return;
                }
        for (ProfessionSkillInfo const& info : currentProfessions)
        {
            if (info.isPrimary)
                majorProfessionCount++;
        }

        // Enforce 2-major profession limit
        if (majorProfessionCount >= 2)
        {
            TC_LOG_WARN("playerbots", "Player {} already has 2 major professions, cannot learn {}",
                player->GetName(), skillId);
            return false;
        }
    }

    // Learn profession (set skill to 1, max based on level)
    uint16 maxSkill = std::min<uint16>(player->GetLevel() * 5, 450);
    if (!player)
    {
        TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetLevel");
        return nullptr;
    }
        if (!player)
        {
            TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetName");
            return nullptr;
        }
    if (!player)
    {
        TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetGUID");
        return;
    }
    player->SetSkill(skillId, 1, 1, maxSkill);

    TC_LOG_DEBUG("playerbots", "Player {} learned profession {} (max skill: {})",
        player->GetName(), skillId, maxSkill);

    // Update metrics
    // No lock needed - profession data is per-bot instance data
    _playerMetrics[player->GetGUID().GetCounter()].professionsLearned++;
    _globalMetrics.professionsLearned++;

    return true;
}

bool ProfessionManager::HasProfession(::Player* player, ProfessionType profession) const
{
    if (!player || profession == ProfessionType::NONE)
        return false;

    return player->HasSkill(static_cast<uint32>(profession));
}

uint16 ProfessionManager::GetProfessionSkill(::Player* player, ProfessionType profession) const
{
    if (!player || profession == ProfessionType::NONE)
        return 0;

    return player->GetSkillValue(static_cast<uint32>(profession));
}

uint16 ProfessionManager::GetMaxProfessionSkill(::Player* player, ProfessionType profession) const
{
    if (!player || profession == ProfessionType::NONE)
        return 0;

    return player->GetMaxSkillValue(static_cast<uint32>(profession));
}

std::vector<ProfessionSkillInfo> ProfessionManager::GetPlayerProfessions(::Player* player) const
{
    std::vector<ProfessionSkillInfo> professions;

    if (!player)
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
        if (!player)
        {
            TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetName");
            return nullptr;
        }
        if (HasProfession(player, profession))
        {
            ProfessionSkillInfo info;
            info.profession = profession;
            info.currentSkill = GetProfessionSkill(player, profession);
            info.maxSkill = GetMaxProfessionSkill(player, profession);
            if (!player)
            {
                TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetClass");
                return;
            }
if (!player)
{
    TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetRace");
    return;
}
            info.lastUpdate = getMSTime();
            info.isPrimary = GetProfessionCategory(profession) != ProfessionCategory::SECONDARY;

            professions.push_back(info);
        }
    }

    return professions;
}

bool ProfessionManager::UnlearnProfession(::Player* player, ProfessionType profession)
{
    if (!player || profession == ProfessionType::NONE)
if (!player)
{
    TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetName");
    return;
}
        if (!player)
        {
            TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetName");
            return;
        }
        return false;

    uint32 skillId = static_cast<uint32>(profession);

    if (!player->HasSkill(skillId))
        return false;

    // Set skill to 0 to unlearn
    player->SetSkill(skillId, 0, 0, 0);

    TC_LOG_DEBUG("playerbots", "Player {} unlearned profession {}",
        player->GetName(), skillId);

    return true;
}

// ============================================================================
// AUTO-LEARN SYSTEM
// ============================================================================

void ProfessionManager::AutoLearnProfessionsForClass(::Player* player)
{
    if (!player)
        return;

    uint8 classId = player->GetClass();
        if (!player)
        {
            TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetName");
            return nullptr;
        }
    if (!player)
    {
        TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetClass");
        return;
    }
    uint8 raceId = player->GetRace();
    if (!player)
    {
        TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetRace");
        return;
    }
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
                if (!player)
                {
                    TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetName");
                    return;
                }
                player->GetName(), static_cast<uint32>(raceId), static_cast<uint32>(profession), raceBonus);
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
                    if (!player)
                    {
                        TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetName");
                        return;
                    }
                    player->GetName(), static_cast<uint32>(secondProf), static_cast<uint32>(firstProf));
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
        LearnProfession(player, firstProf);
    }

    if (secondProf != ProfessionType::NONE)
    {
        LearnProfession(player, secondProf);
    }

    // Always learn secondary professions (unlimited)
    LearnProfession(player, ProfessionType::COOKING);
    LearnProfession(player, ProfessionType::FISHING);
}

std::vector<ProfessionType> ProfessionManager::GetRecommendedProfessions(uint8 classId) const
{
    auto it = _classRecommendations.find(classId);
    if (it != _classRecommendations.end())
        return it->second;

    return {};
}

bool ProfessionManager::IsProfessionSuitableForClass(uint8 classId, ProfessionType profession) const
{
    std::vector<ProfessionType> recommended = GetRecommendedProfessions(classId);
    return std::find(recommended.begin(), recommended.end(), profession) != recommended.end();
}

ProfessionCategory ProfessionManager::GetProfessionCategory(ProfessionType profession) const
{
    switch (profession)
    {
        case ProfessionType::MINING:
        case ProfessionType::HERBALISM:
        case ProfessionType::SKINNING:
            return ProfessionCategory::GATHERING;

        case ProfessionType::COOKING:
        case ProfessionType::FISHING:
        case ProfessionType::FIRST_AID:
            return ProfessionCategory::SECONDARY;

        default:
            return ProfessionCategory::PRODUCTION;
    }
}

std::vector<ProfessionType> ProfessionManager::GetBeneficialPairs(ProfessionType profession) const
{
    auto it = _professionPairs.find(profession);
    if (it != _professionPairs.end())
        return it->second;

    return {};
}

bool ProfessionManager::IsBeneficialPair(ProfessionType prof1, ProfessionType prof2) const
{
    std::vector<ProfessionType> pairs1 = GetBeneficialPairs(prof1);
    if (std::find(pairs1.begin(), pairs1.end(), prof2) != pairs1.end())
        return true;

    std::vector<ProfessionType> pairs2 = GetBeneficialPairs(prof2);
    if (std::find(pairs2.begin(), pairs2.end(), prof1) != pairs2.end())
        return true;

    return false;
}

uint16 ProfessionManager::GetRaceProfessionBonus(uint8 raceId, ProfessionType profession) const
{
    auto raceIt = _raceBonuses.find(raceId);
    if (raceIt == _raceBonuses.end())
        return 0;

    auto profIt = raceIt->second.find(profession);
    if (profIt != raceIt->second.end())
        return profIt->second;

    return 0;
}

void ProfessionManager::InitializeProfessionPairs()
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

    TC_LOG_DEBUG("playerbots", "ProfessionManager: Initialized {} beneficial profession pairs",
        _professionPairs.size());
}

void ProfessionManager::InitializeRaceBonuses()
{
    _raceBonuses.clear();

    if (!player)
    {
        TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetName");
        return;
    }
    // WoW 11.2 Racial Profession Bonuses

    // TAUREN (+15 Herbalism)
    _raceBonuses[RACE_TAUREN][ProfessionType::HERBALISM] = 15;

    // BLOOD ELF (+10 Enchanting)
    _raceBonuses[RACE_BLOODELF][ProfessionType::ENCHANTING] = 10;

    // DRAENEI (+10 Jewelcrafting)
    if (!player)
    {
        TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetName");
        return nullptr;
    }
    _raceBonuses[RACE_DRAENEI][ProfessionType::JEWELCRAFTING] = 10;

    // WORGEN (+15 Skinning)
    _raceBonuses[RACE_WORGEN][ProfessionType::SKINNING] = 15;

    if (!player)
    {
        TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method LearnSpell");
        return nullptr;
    }
    // GOBLIN (+15 Alchemy)
    _raceBonuses[RACE_GOBLIN][ProfessionType::ALCHEMY] = 15;

    if (!player)
    {
        TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetName");
        return nullptr;
    }
    // PANDAREN (+15 Cooking)
    _raceBonuses[RACE_PANDAREN_NEUTRAL][ProfessionType::COOKING] = 15;
    _raceBonuses[RACE_PANDAREN_ALLIANCE][ProfessionType::COOKING] = 15;
    _raceBonuses[RACE_PANDAREN_HORDE][ProfessionType::COOKING] = 15;
if (!player)
{
    TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetGUID");
    return nullptr;
}

    // DARK IRON DWARF (+5 Blacksmithing) - Constant may not exist in 11.2, commenting out
    // _raceBonuses[RACE_DARKIRONDWARF][ProfessionType::BLACKSMITHING] = 5;

    // KUL TIRAN (+5 Fishing, +5 Cooking) - Constant may not exist in 11.2, commenting out
    // _raceBonuses[RACE_KULTIRAN][ProfessionType::FISHING] = 5;
    // _raceBonuses[RACE_KULTIRAN][ProfessionType::COOKING] = 5;

    TC_LOG_DEBUG("playerbots", "ProfessionManager: Initialized racial bonuses for {} races",
        _raceBonuses.size());
}

// ============================================================================
// RECIPE MANAGEMENT
// ============================================================================
if (!player)
{
    TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method HasSpell");
    return;
}

bool ProfessionManager::LearnRecipe(::Player* player, uint32 recipeId)
            if (!player)
            {
                TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetName");
                return;
            }
{
    if (!player)
        return false;

    auto it = _recipeDatabase.find(recipeId);
    if (it == _recipeDatabase.end())
    {
        TC_LOG_WARN("playerbots", "Unknown recipe ID: {}", recipeId);
        return false;
    }

    RecipeInfo const& recipe = it->second;

    // Check if player has the profession
    if (!HasProfession(player, recipe.profession))
    {
        TC_LOG_DEBUG("playerbots", "Player {} doesn't have profession {} for recipe {}",
            player->GetName(), static_cast<uint32>(recipe.profession), recipeId);
        return false;
    }

    // Check skill requirement
    uint16 skill = GetProfessionSkill(player, recipe.profession);
            if (!player)
            {
                TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetName");
                return nullptr;
            }
        if (!player)
        {
            TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method LearnSpell");
            return nullptr;
        }
if (!player)
{
    TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetName");
    return;
}
    if (!player)
    {
        TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetGUID");
        return;
    }
    if (skill < recipe.requiredSkill)
    {
        TC_LOG_DEBUG("playerbots", "Player {} skill {} too low for recipe {} (requires {})",
            player->GetName(), skill, recipeId, recipe.requiredSkill);
        return false;
    }

    // Learn the spell
    player->LearnSpell(recipe.spellId, false);

    TC_LOG_DEBUG("playerbots", "Player {} learned recipe {} (spell {})",
        player->GetName(), recipeId, recipe.spellId);

    // Update metrics
    // No lock needed - profession data is per-bot instance data
    _playerMetrics[player->GetGUID().GetCounter()].recipesLearned++;
    _globalMetrics.recipesLearned++;

    if (!player)
    {
        TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method HasSpell");
        return;
    }
    return true;
}

bool ProfessionManager::KnowsRecipe(::Player* player, uint32 recipeId) const
    if (!player)
    {
        TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method HasSpell");
        return;
    }
{
    if (!player)
        return false;

    auto it = _recipeDatabase.find(recipeId);
    if (it == _recipeDatabase.end())
        return false;

    return player->HasSpell(it->second.spellId);
}

std::vector<RecipeInfo> ProfessionManager::GetRecipesForProfession(ProfessionType profession) const
{
    std::vector<RecipeInfo> recipes;

    auto it = _professionRecipes.find(profession);
    if (it == _professionRecipes.end())
        return recipes;

    for (uint32 recipeId : it->second)
    {
        auto recipeIt = _recipeDatabase.find(recipeId);
        if (recipeIt != _recipeDatabase.end())
            recipes.push_back(recipeIt->second);
    }

    return recipes;
}

std::vector<RecipeInfo> ProfessionManager::GetCraftableRecipes(::Player* player, ProfessionType profession) const
{
    std::vector<RecipeInfo> craftable;

    if (!player)
        return craftable;

    std::vector<RecipeInfo> allRecipes = GetRecipesForProfession(profession);

    for (RecipeInfo const& recipe : allRecipes)
    {
        if (CanCraftRecipe(player, recipe))
            craftable.push_back(recipe);
    }

    return craftable;
}

RecipeInfo const* ProfessionManager::GetOptimalLevelingRecipe(::Player* player, ProfessionType profession) const
{
    if (!player)
        return nullptr;

    std::vector<RecipeInfo> craftable = GetCraftableRecipes(player, profession);

    RecipeInfo const* best = nullptr;
    float bestChance = 0.0f;

    for (RecipeInfo const& recipe : craftable)
    {
        float chance = GetSkillUpChance(player, recipe);
    if (!player)
    {
        TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetGUID");
        return nullptr;
    }
        if (chance > bestChance)
        {
            bestChance = chance;
            best = &recipe;
        }
    }

    return best;
}

bool ProfessionManager::CanCraftRecipe(::Player* player, RecipeInfo const& recipe) const
    if (!player)
    {
        TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetName");
        return;
    }
    if (!player)
    {
        TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetGUID");
        return nullptr;
    }
    if (!player)
    {
        TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method HasSpell");
        return nullptr;
    }
{
    if (!player)
        return false;

    // Must know the recipe
    if (!player->HasSpell(recipe.spellId))
        return false;

    // Must have skill
    uint16 skill = GetProfessionSkill(player, recipe.profession);
    if (skill < recipe.requiredSkill)
        return false;

    // Must have materials
    return HasMaterialsForRecipe(player, recipe);
}

float ProfessionManager::GetSkillUpChance(::Player* player, RecipeInfo const& recipe) const
{
    if (!player)
        return 0.0f;

    uint16 skill = GetProfessionSkill(player, recipe.profession);

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

bool ProfessionManager::AutoLevelProfession(::Player* player, ProfessionType profession)
{
    if (!player)
        return false;

    RecipeInfo const* recipe = GetOptimalLevelingRecipe(player, profession);
    if (!recipe)
    {
        TC_LOG_DEBUG("playerbots", "No optimal leveling recipe found for profession {}",
            static_cast<uint32>(profession));
        return false;
    }

    return CraftItem(player, *recipe, 1);
}

bool ProfessionManager::CraftItem(::Player* player, RecipeInfo const& recipe, uint32 quantity)
{
    if (!player || quantity == 0)
        return false;

    if (!CanCraftRecipe(player, recipe))
        return false;

    // Queue the craft
    QueueCraft(player, recipe.recipeId, quantity);

    return true;
}

void ProfessionManager::QueueCraft(::Player* player, uint32 recipeId, uint32 quantity)
{
    if (!player || quantity == 0)
        return;

    uint32 playerGuid = player->GetGUID().GetCounter();
    if (!player)
    {
        TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetGUID");
        return;
    }
        if (!player)
        {
            TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetName");
            return;
        }

    CraftingTask task;
    task.recipeId = recipeId;
    task.quantity = quantity;
    task.queueTime = getMSTime();

    // No lock needed - profession data is per-bot instance data
    _craftingQueues[playerGuid].push_back(task);

    TC_LOG_DEBUG("playerbots", "Queued {} x{} for player {}",
        recipeId, quantity, player->GetName());
}

void ProfessionManager::ProcessCraftingQueue(::Player* player, uint32 diff)
{
    if (!player)
        return;

    uint32 playerGuid = player->GetGUID().GetCounter();
if (!player)
{
    TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetGUID");
    return;
}
    if (!player)
    {
        TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetGUID");
        return;
    }

    // No lock needed - profession data is per-bot instance data
    auto it = _craftingQueues.find(playerGuid);
    if (it == _craftingQueues.end() || it->second.empty())
        return;

    CraftingTask& task = it->second.front();

    auto recipeIt = _recipeDatabase.find(task.recipeId);
    if (recipeIt == _recipeDatabase.end())
    {
        // Invalid recipe, remove from queue
        it->second.erase(it->second.begin());
        return;
    }

    RecipeInfo const& recipe = recipeIt->second;

    // Check if we can craft
    if (!CanCraftRecipe(player, recipe))
    {
        TC_LOG_DEBUG("playerbots", "Cannot craft recipe {}, removing from queue", task.recipeId);
        it->second.erase(it->second.begin());
        return;
    }

    // Craft one item
    if (CastCraftingSpell(player, recipe))
    {
        task.quantity--;

        // Update metrics
        _playerMetrics[playerGuid].itemsCrafted++;
        _globalMetrics.itemsCrafted++;
if (!player)
{
    TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetName");
    return;
}

        TC_LOG_DEBUG("playerbots", "Crafted 1x {} ({} remaining in queue)",
            task.recipeId, task.quantity);

        // Remove if finished
        if (task.quantity == 0)
            it->second.erase(it->second.begin());
    }
}

bool ProfessionManager::HasMaterialsForRecipe(::Player* player, RecipeInfo const& recipe) const
    if (!player)
    {
        TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetGUID");
        return nullptr;
    }
{
    if (!player)
        return false;

    for (RecipeInfo::Reagent const& reagent : recipe.reagents)
    {
        uint32 count = player->GetItemCount(reagent.itemId);
        if (count < reagent.quantity)
            return false;
    }

    return true;
}

std::vector<std::pair<uint32, uint32>> ProfessionManager::GetMissingMaterials(::Player* player, RecipeInfo const& recipe) const
{
    std::vector<std::pair<uint32, uint32>> missing;

    if (!player)
        return missing;

    for (RecipeInfo::Reagent const& reagent : recipe.reagents)
    {
        uint32 have = player->GetItemCount(reagent.itemId);
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

bool ProfessionManager::CastCraftingSpell(::Player* player, RecipeInfo const& recipe)
{
    if (!player)
        return false;

    // Consume materials
    if (!ConsumeMaterials(player, recipe))
        return false;

    // Cast crafting spell
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(recipe.spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return false;

    // Update profession skill
    player->UpdateCraftSkill(spellInfo);

    // Skill point gain
    uint16 oldSkill = GetProfessionSkill(player, recipe.profession);
    uint16 newSkill = GetProfessionSkill(player, recipe.profession);
    if (newSkill > oldSkill)
    {
        // No lock needed - profession data is per-bot instance data
        _playerMetrics[player->GetGUID().GetCounter()].skillPointsGained += (newSkill - oldSkill);
        if (!player)
        {
            TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetGUID");
            return;
        }
        _globalMetrics.skillPointsGained += (newSkill - oldSkill);
    }

    HandleCraftingResult(player, recipe, true);

    return true;
}

bool ProfessionManager::ConsumeMaterials(::Player* player, RecipeInfo const& recipe)
{
    if (!player)
        return false;

    // Check we have all materials first
    if (!HasMaterialsForRecipe(player, recipe))
        return false;

    // Destroy reagents
    for (RecipeInfo::Reagent const& reagent : recipe.reagents)
    {
        player->DestroyItemCount(reagent.itemId, reagent.quantity, true);
    }

    return true;
}

void ProfessionManager::HandleCraftingResult(::Player* player, RecipeInfo const& recipe, bool success)
        if (!player)
        {
            TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetName");
            return;
        }
{
    if (!player || !success)
        return;

    // Item will be created by spell effect, we don't create it manually
    TC_LOG_DEBUG("playerbots", "Player {} successfully crafted item {} from recipe {}",
        player->GetName(), recipe.productItemId, recipe.recipeId);
}

// ============================================================================
// SKILL CALCULATION HELPERS
// ============================================================================

uint16 ProfessionManager::CalculateSkillUpAmount(RecipeInfo const& recipe, uint16 currentSkill) const
{
    // Simple skill-up: 1 point per craft
    return 1;
}

bool ProfessionManager::ShouldCraftForSkillUp(::Player* player, RecipeInfo const& recipe) const
{
    float chance = GetSkillUpChance(player, recipe);
    ProfessionAutomationProfile const& profile = GetAutomationProfile(player->GetGUID().GetCounter());
    if (!player)
    {
        TC_LOG_ERROR("playerbot.nullcheck", "Null pointer: player in method GetGUID");
        return nullptr;
    }

    return chance >= profile.skillUpThreshold;
}

// ============================================================================
// AUTOMATION PROFILES
// ============================================================================

void ProfessionManager::SetAutomationProfile(uint32 playerGuid, ProfessionAutomationProfile const& profile)
{
    // No lock needed - profession data is per-bot instance data
    _playerProfiles[playerGuid] = profile;
}

ProfessionAutomationProfile ProfessionManager::GetAutomationProfile(uint32 playerGuid) const
{
    // No lock needed - profession data is per-bot instance data
    auto it = _playerProfiles.find(playerGuid);
    if (it != _playerProfiles.end())
        return it->second;

    // Return default profile
    return ProfessionAutomationProfile();
}

// ============================================================================
// METRICS
// ============================================================================

ProfessionManager::ProfessionMetrics const& ProfessionManager::GetPlayerMetrics(uint32 playerGuid) const
{
    // No lock needed - profession data is per-bot instance data
    auto it = _playerMetrics.find(playerGuid);
    if (it != _playerMetrics.end())
        return it->second;

    // Return default metrics if not found
    static ProfessionMetrics defaultMetrics;
    return defaultMetrics;
}

ProfessionManager::ProfessionMetrics const& ProfessionManager::GetGlobalMetrics() const
{
    return _globalMetrics;
}

} // namespace Playerbot
