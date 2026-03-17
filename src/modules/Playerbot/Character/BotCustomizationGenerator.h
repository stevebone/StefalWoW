/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 */

#ifndef BOT_CUSTOMIZATION_GENERATOR_H
#define BOT_CUSTOMIZATION_GENERATOR_H

#include "Define.h"
#include "SharedDefines.h"
#include "UpdateFields.h"
#include <array>
#include <vector>
#include <random>
#include <unordered_map>

// Forward declarations
struct ChrCustomizationOptionEntry;
struct ChrCustomizationChoiceEntry;

namespace WorldPackets {
namespace Character {
    using ChrCustomizationChoice = UF::ChrCustomizationChoice;
}
}

namespace Playerbot {

/**
 * @brief Bot Customization Generator
 *
 * Generates realistic visual customizations for bot characters by:
 * 1. Querying valid customization options from DBC/DB2 stores
 * 2. Randomly selecting appropriate choices for race/gender combinations
 * 3. Ensuring all generated customizations are valid and lore-appropriate
 *
 * INTEGRATION STRATEGY:
 * - Uses TrinityCore's DBC/DB2 stores for validation
 * - Generates Array<ChrCustomizationChoice, 250> for CharacterCreateInfo
 * - Lets TrinityCore handle all validation through existing systems
 */
class TC_GAME_API BotCustomizationGenerator
{
public:
    /**
     * @brief Generate customizations for a bot character
     * @param race Character race ID
     * @param gender Character gender (GENDER_MALE/FEMALE)
     * @return Array of customization choices ready for character creation
     */
    static ::std::array<WorldPackets::Character::ChrCustomizationChoice, 250>
        GenerateCustomizations(uint8 race, uint8 gender);

    /**
     * @brief Initialize the customization cache system
     * Called once during server startup to cache DBC data
     */
    static void Initialize();

private:
    struct CustomizationOption {
        uint32 optionId;
        ::std::vector<uint32> availableChoices;
        bool isRequired;
        uint32 defaultChoice;
    };

    // Cache of valid customization options per race/gender
    inline static ::std::unordered_map<uint64, ::std::vector<CustomizationOption>> _customizationCache;
    inline static bool _initialized = false;

    /**
     * @brief Get cache key for race/gender combination
     */
    static uint64 GetCacheKey(uint8 race, uint8 gender)
    {
        return (uint64(race) << 8) | uint64(gender);
    }

    /**
     * @brief Load customization options for specific race/gender from DBC
     */
    static void LoadCustomizationOptions(uint8 race, uint8 gender);

    /**
     * @brief Get valid customization choices for an option
     */
    static ::std::vector<uint32> GetValidChoicesForOption(uint32 optionId);

    /**
     * @brief Select random choice from available options
     */
    static uint32 GetRandomChoice(::std::vector<uint32> const& choices);

    /**
     * @brief Check if customization option is required
     */
    static bool IsRequiredOption(uint32 optionId);

    /**
     * @brief Get default choice for an option (fallback)
     */
    static uint32 GetDefaultChoice(uint32 optionId);

    /**
     * @brief Validate generated customizations against DBC requirements
     */
    static bool ValidateCustomizations(uint8 race, uint8 gender,
        ::std::array<WorldPackets::Character::ChrCustomizationChoice, 250> const& customizations);
};

} // namespace Playerbot

#endif // BOT_CUSTOMIZATION_GENERATOR_H