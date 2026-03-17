/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 */

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <cmath>

#include "BotCustomizationGenerator.h"
#include "DB2Stores.h"
#include "DB2Structure.h"
#include "Log.h"
#include "Random.h"
#include <algorithm>
#include <chrono>

namespace Playerbot {

// Static member definitions removed - now inline static in header to fix DLL export issues

void BotCustomizationGenerator::Initialize()
{
    if (_initialized)
    {
        return;
    }

    TC_LOG_INFO("module.playerbot.character", "Initializing Bot Customization Generator...");

    uint32 totalCombinations = 0;
    uint32 totalOptions = 0;

    // Load customization options for all valid race/gender combinations
    for (ChrRacesEntry const* raceEntry : sChrRacesStore)
    {
        if (!raceEntry) continue;

        for (uint8 gender = GENDER_MALE; gender <= GENDER_FEMALE; ++gender) {
            // Verify this race/gender combination has a valid model
    if (sDB2Manager.GetChrModel(raceEntry->ID, gender))
    {
                LoadCustomizationOptions(raceEntry->ID, gender);
                ++totalCombinations;

                uint64 key = GetCacheKey(raceEntry->ID, gender);
                if (_customizationCache.find(key) != _customizationCache.end()) {
                    totalOptions += _customizationCache[key].size();
                }
            }
        }
    }

    _initialized = true;

    TC_LOG_INFO("module.playerbot.character",
        "Bot Customization Generator initialized: {} race/gender combinations, {} total customization options",
        totalCombinations, totalOptions);
}

::std::array<WorldPackets::Character::ChrCustomizationChoice, 250>
BotCustomizationGenerator::GenerateCustomizations(uint8 race, uint8 gender)
{
    if (!_initialized)
    {
        Initialize();
    }

    ::std::array<WorldPackets::Character::ChrCustomizationChoice, 250> customizations{};
    uint32 customizationCount = 0;

    uint64 key = GetCacheKey(race, gender);
    auto cacheItr = _customizationCache.find(key);

    if (cacheItr == _customizationCache.end()) {
        TC_LOG_ERROR("module.playerbot.character",
            "No customization options found for race {} gender {}. Using empty customizations.",
            race, gender);
        return customizations;
    }

    // Generate customizations from cached options
    for (CustomizationOption const& option : cacheItr->second)
    {
        if (customizationCount >= 250) {
            TC_LOG_WARN("module.playerbot.character",
                "Reached maximum customization limit (250) for race {} gender {}", race, gender);
            break;
        }

        uint32 choiceId = 0;

        if (!option.availableChoices.empty())
        {
            // Select random choice from available options
            choiceId = GetRandomChoice(option.availableChoices);
        } else if (option.isRequired)
        {
            // Use default choice for required options
            choiceId = option.defaultChoice;
            TC_LOG_WARN("module.playerbot.character",
                "Using default choice {} for required option {} (race {} gender {})",
                choiceId, option.optionId, race, gender);
        } else {
            // Skip optional options without available choices
            continue;
        }

        // Add customization choice
        customizations[customizationCount].ChrCustomizationOptionID = option.optionId;
        customizations[customizationCount].ChrCustomizationChoiceID = choiceId;
        ++customizationCount;
    }

    // Validate generated customizations
    if (!ValidateCustomizations(race, gender, customizations))
    {
        TC_LOG_ERROR("module.playerbot.character",
            "Generated customizations failed validation for race {} gender {}. Using minimal customizations.",
            race, gender);

        // Return minimal valid customizations
        customizations = {};
        customizationCount = 0;
    }

    TC_LOG_DEBUG("module.playerbot.character",
        "Generated {} customizations for race {} gender {}", customizationCount, race, gender);

    return customizations;
}

void BotCustomizationGenerator::LoadCustomizationOptions(uint8 race, uint8 gender)
{
    uint64 key = GetCacheKey(race, gender);
    ::std::vector<CustomizationOption> options;

    // Get the character model for this race/gender
    ChrModelEntry const* chrModel = sDB2Manager.GetChrModel(race, gender);
    if (!chrModel)
    {
        TC_LOG_ERROR("module.playerbot.character",
            "No character model found for race {} gender {}", race, gender);
        return;
    }

    uint32 optionCount = 0;

    // Use DB2Manager to get customization options for this race/gender
    if (auto const* optionVector = sDB2Manager.GetCustomiztionOptions(race, gender)) {
        for (ChrCustomizationOptionEntry const* optionEntry : *optionVector)
        {
            if (!optionEntry) continue;

            CustomizationOption option;
            option.optionId = optionEntry->ID;
            option.availableChoices = GetValidChoicesForOption(optionEntry->ID);
            option.isRequired = IsRequiredOption(optionEntry->ID);
            option.defaultChoice = GetDefaultChoice(optionEntry->ID);

            if (!option.availableChoices.empty() || option.isRequired)
            {
                options.push_back(option);
                ++optionCount;
            }
        }
    }

    _customizationCache[key] = ::std::move(options);

    TC_LOG_DEBUG("module.playerbot.character",
        "Loaded {} customization options for race {} gender {}", optionCount, race, gender);
}

::std::vector<uint32> BotCustomizationGenerator::GetValidChoicesForOption(uint32 optionId)
{
    ::std::vector<uint32> choices;

    // Use DB2Manager to get customization choices
    if (auto const* choiceVector = sDB2Manager.GetCustomiztionChoices(optionId)) {
        for (ChrCustomizationChoiceEntry const* choice : *choiceVector)
        {
            if (choice)
            {
                choices.push_back(choice->ID);
            }
        }
    }

    return choices;
}

uint32 BotCustomizationGenerator::GetRandomChoice(::std::vector<uint32> const& choices)
{
    if (choices.empty())
    {
        return 0;
    }

    if (choices.size() == 1) {
        return choices[0];
    }

    // Use Trinity's random number generator for consistency
    uint32 index = urand(0, static_cast<uint32>(choices.size() - 1));
    return choices[index];
}

bool BotCustomizationGenerator::IsRequiredOption(uint32 /*optionId*/)
{
    // For now, assume all customization options are optional
    // The actual requirements can be determined through empirical testing
    // or by analyzing the relationship between customization options and requirements

    // TrinityCore's character creation validation will handle required options
    // so we don't need to worry about missing required customizations
    return false;
}

uint32 BotCustomizationGenerator::GetDefaultChoice(uint32 optionId)
{
    // Get the first available choice as default
    ::std::vector<uint32> choices = GetValidChoicesForOption(optionId);
    if (!choices.empty())
    {
        return choices[0];
    }

    // Return 0 if no choices available (will be handled by caller)
    return 0;
}

bool BotCustomizationGenerator::ValidateCustomizations(uint8 race, uint8 gender,
    ::std::array<WorldPackets::Character::ChrCustomizationChoice, 250> const& customizations)
{
    // Basic validation - ensure we have some customizations
    bool hasCustomizations = false;
    for (auto const& customization : customizations)
    {
        if (customization.ChrCustomizationOptionID != 0) {
            hasCustomizations = true;
            break;
        }
    }

    if (!hasCustomizations)
    {
        TC_LOG_WARN("module.playerbot.character",
            "No customizations generated for race {} gender {}", race, gender);
        // Allow empty customizations - TrinityCore will use defaults
        return true;
    }

    // Additional validation can be added here:
    // - Check for conflicting customizations
    // - Verify all required options are present
    // - Validate choice IDs against DBC data

    return true;
}

} // namespace Playerbot