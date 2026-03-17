/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_BOT_CHARACTER_CREATOR_H
#define PLAYERBOT_BOT_CHARACTER_CREATOR_H

#include "Define.h"
#include "ObjectGuid.h"
#include "SharedDefines.h"
#include <string>

namespace Playerbot
{

/**
 * @brief Handles character creation for PlayerBot system
 *
 * This class provides functionality to create new bot characters programmatically,
 * handling all database operations, validation, and TrinityCore integration.
 */
class TC_GAME_API BotCharacterCreator
{
public:
    /**
     * @brief Result of character creation attempt
     */
    enum class CreateResult : uint8
    {
        SUCCESS = 0,
        INVALID_RACE_CLASS_COMBO,
        INVALID_NAME,
        NAME_IN_USE,
        NAME_RESERVED,
        ACCOUNT_LIMIT,
        REALM_LIMIT,
        EVOKER_LIMIT,
        DEMON_HUNTER_LIMIT,
        DATABASE_ERROR,
        CREATION_FAILED,
        UNKNOWN_ERROR
    };

    /**
     * @brief Create a new bot character
     *
     * @param accountId Account ID to create the character under
     * @param race Character race (Races enum)
     * @param classId Character class (Classes enum)
     * @param gender Character gender (Gender enum - GENDER_MALE or GENDER_FEMALE)
     * @param name Character name (must be valid and unique)
     * @param outGuid [out] Receives the created character's GUID on success
     * @param outErrorMsg [out] Receives detailed error message on failure
     * @return CreateResult indicating success or specific failure reason
     */
    static CreateResult CreateBotCharacter(
        uint32 accountId,
        uint8 race,
        uint8 classId,
        uint8 gender,
        ::std::string const& name,
        ObjectGuid& outGuid,
        ::std::string& outErrorMsg);

    /**
     * @brief Validate race/class combination
     * @return true if combination is valid in WoW 12.0
     */
    static bool IsValidRaceClassCombination(uint8 race, uint8 classId);

    /**
     * @brief Check if account has reached character limit
     * @param accountId Account to check
     * @param currentCount [out] Current character count
     * @return true if account can create more characters
     */
    static bool CanCreateCharacter(uint32 accountId, uint32& currentCount);

    /**
     * @brief Generate default random bot name
     * @param race Character race
     * @param gender Character gender
     * @return Generated name string
     */
    static ::std::string GenerateDefaultBotName(uint8 race, uint8 gender);

    /**
     * @brief Get starting level for race/class combination
     */
    static uint8 GetStartingLevel(uint8 race, uint8 classId);

    /**
     * @brief Convert CreateResult to human-readable string
     */
    static char const* ResultToString(CreateResult result);

private:
    // Internal creation steps
    static CreateResult ValidateCreationRequest(
        uint32 accountId,
        uint8 race,
        uint8 classId,
        uint8 gender,
        ::std::string const& name,
        ::std::string& outErrorMsg);

    static CreateResult CreatePlayerObject(
        uint32 accountId,
        uint8 race,
        uint8 classId,
        uint8 gender,
        ::std::string const& name,
        ObjectGuid& outGuid,
        ::std::string& outErrorMsg);
};

} // namespace Playerbot

#endif // PLAYERBOT_BOT_CHARACTER_CREATOR_H
