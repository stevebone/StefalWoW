/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "BotCharacterCreator.h"
#include "Character/BotNameMgr.h"
#include "CharacterCache.h"
#include "CharacterDatabase.h"
#include "DatabaseEnv.h"
#include "DB2Stores.h"
#include "Log.h"
#include "LoginDatabase.h"
#include "MotionMaster.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "RealmList.h"
#include "ScriptMgr.h"
#include "World.h"
#include "WorldSession.h"
#include "../Session/BotSession.h"
#include "../AI/BotAI.h"  // P1 FIX: Required for unique_ptr<BotAI> in BotSession.h
#include "Data/WoW120CharacterCreation.h"
#include "CharacterPackets.h"
#include <random>

namespace Playerbot
{

// ===================================================================================
// PUBLIC API IMPLEMENTATION
// ===================================================================================

BotCharacterCreator::CreateResult BotCharacterCreator::CreateBotCharacter(
    uint32 accountId,
    uint8 race,
    uint8 classId,
    uint8 gender,
    ::std::string const& name,
    ObjectGuid& outGuid,
    ::std::string& outErrorMsg)
{
    // Phase 1: Validation
    CreateResult validationResult = ValidateCreationRequest(
        accountId, race, classId, gender, name, outErrorMsg);

    if (validationResult != CreateResult::SUCCESS)
        return validationResult;

    // Phase 2: Create Player object
    return CreatePlayerObject(accountId, race, classId, gender, name, outGuid, outErrorMsg);
}

bool BotCharacterCreator::IsValidRaceClassCombination(uint8 race, uint8 classId)
{
    // Check using WoW 12.0 data
    for (auto const& combo : CharacterCreation::VALID_COMBINATIONS)
    {
        if (combo.race == static_cast<Races>(race) &&
            combo.playerClass == static_cast<Classes>(classId))
            return true;
    }
    return false;
}

bool BotCharacterCreator::CanCreateCharacter(uint32 accountId, uint32& currentCount)
{
    // Query current character count for account
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_SUM_CHARS);
    stmt->setUInt32(0, accountId);

    PreparedQueryResult result = CharacterDatabase.Query(stmt);

    currentCount = 0;
    if (result)
    {
        Field* fields = result->Fetch();
        currentCount = fields[0].GetUInt32();
    }

    uint32 maxChars = sWorld->getIntConfig(CONFIG_CHARACTERS_PER_REALM);
    return currentCount < maxChars;
}

::std::string BotCharacterCreator::GenerateDefaultBotName(uint8 race, uint8 gender)
{
    // Delegate to BotNameMgr which handles:
    // 1. Name generation with fantasy-style prefixes/suffixes
    // 2. Collision detection against characters table
    // 3. Automatic retry on collision (up to 100 attempts)
    //
    // This prevents duplicate name errors when creating bots

    std::string uniqueName = sBotNameMgr->GenerateUniqueName(gender);

    if (uniqueName.empty())
    {
        TC_LOG_ERROR("module.playerbot",
            "BotCharacterCreator: Failed to generate unique name for race={} gender={}",
            race, gender);
    }

    return uniqueName;
}

uint8 BotCharacterCreator::GetStartingLevel(uint8 race, uint8 classId)
{
    if (classId == CLASS_EVOKER)
        return CharacterCreation::EVOKER_STARTING_LEVEL;

    if (classId == CLASS_DEATH_KNIGHT)
        return CharacterCreation::DEATH_KNIGHT_STARTING_LEVEL;

    if (classId == CLASS_DEMON_HUNTER)
        return CharacterCreation::DEMON_HUNTER_STARTING_LEVEL;

    // Check if allied race
    for (auto const& combo : CharacterCreation::VALID_COMBINATIONS)
    {
        if (combo.race == static_cast<Races>(race) && combo.isAlliedRace)
            return CharacterCreation::ALLIED_RACE_STARTING_LEVEL;
    }

    return CharacterCreation::DEFAULT_STARTING_LEVEL;
}

char const* BotCharacterCreator::ResultToString(CreateResult result)
{
    switch (result)
    {
        case CreateResult::SUCCESS: return "Success";
        case CreateResult::INVALID_RACE_CLASS_COMBO: return "Invalid race/class combination";
        case CreateResult::INVALID_NAME: return "Invalid character name";
        case CreateResult::NAME_IN_USE: return "Name already in use";
        case CreateResult::NAME_RESERVED: return "Name is reserved";
        case CreateResult::ACCOUNT_LIMIT: return "Account character limit reached";
        case CreateResult::REALM_LIMIT: return "Realm character limit reached";
        case CreateResult::EVOKER_LIMIT: return "Evoker limit reached for this realm";
        case CreateResult::DEMON_HUNTER_LIMIT: return "Demon Hunter limit reached for this realm";
        case CreateResult::DATABASE_ERROR: return "Database error";
        case CreateResult::CREATION_FAILED: return "Character creation failed";
        default: return "Unknown error";
    }
}

// ===================================================================================
// PRIVATE IMPLEMENTATION
// ===================================================================================

BotCharacterCreator::CreateResult BotCharacterCreator::ValidateCreationRequest(
    uint32 accountId,
    uint8 race,
    uint8 classId,
    uint8 gender,
    ::std::string const& name,
    ::std::string& outErrorMsg)
{
    // 1. Validate race/class combination
    if (!IsValidRaceClassCombination(race, classId))
    {
        outErrorMsg = "Invalid race/class combination for WoW 12.0";
        return CreateResult::INVALID_RACE_CLASS_COMBO;
    }

    // 2. Validate gender
    if (gender != GENDER_MALE && gender != GENDER_FEMALE)
    {
        outErrorMsg = "Invalid gender (must be GENDER_MALE or GENDER_FEMALE)";
        return CreateResult::INVALID_NAME;
    }

    // 3. Validate name
    ::std::string nameToCheck = name;
    if (!normalizePlayerName(nameToCheck))
    {
        outErrorMsg = "Character name is empty or invalid";
        return CreateResult::INVALID_NAME;
    }

    ResponseCodes nameCheck = ObjectMgr::CheckPlayerName(nameToCheck, DEFAULT_LOCALE, true);
    if (nameCheck != CHAR_NAME_SUCCESS)
    {
        outErrorMsg = "Character name validation failed";
        return CreateResult::INVALID_NAME;
    }

    // 4. Check if name is reserved
    if (sObjectMgr->IsReservedName(nameToCheck))
    {
        outErrorMsg = "Character name is reserved";
        return CreateResult::NAME_RESERVED;
    }

    // 5. Check if name already exists
    if (sCharacterCache->GetCharacterCacheByName(nameToCheck))
    {
        outErrorMsg = "Character name already in use";
        return CreateResult::NAME_IN_USE;
    }

    // 6. Check account character limit
    uint32 currentCount = 0;
    if (!CanCreateCharacter(accountId, currentCount))
    {
        outErrorMsg = "Account has reached character limit (";
        outErrorMsg += ::std::to_string(sWorld->getIntConfig(CONFIG_CHARACTERS_PER_REALM));
        outErrorMsg += ")";
        return CreateResult::REALM_LIMIT;
    }

    // 7. Skip hero class limits for bot accounts
    // Note: CHAR_SEL_CHAR_CREATE_INFO is registered as CONNECTION_ASYNC only,
    // but bot creation runs synchronously. Since bots are controlled accounts
    // without player-facing limits, we skip this check entirely.
    // For real players, the WorldSession login process handles these limits.

    return CreateResult::SUCCESS;
}

BotCharacterCreator::CreateResult BotCharacterCreator::CreatePlayerObject(
    uint32 accountId,
    uint8 race,
    uint8 classId,
    uint8 gender,
    ::std::string const& name,
    ObjectGuid& outGuid,
    ::std::string& outErrorMsg)
{
    // Create a BotSession for character creation
    // CRITICAL: Must use BotSession (not WorldSession) because BotSession overrides
    // SendPacket() to handle the null socket case. Using raw WorldSession would cause
    // "non existent socket" errors during Player::Create() which sends packets.
    ::std::shared_ptr<BotSession> tempSession = BotSession::Create(accountId);

    // Build CharacterCreateInfo structure
    auto createInfo = ::std::make_shared<WorldPackets::Character::CharacterCreateInfo>();
    createInfo->Race = race;
    createInfo->Class = classId;
    createInfo->Sex = gender;
    createInfo->Name = name;
    createInfo->IsTrialBoost = false;
    createInfo->UseNPE = false;
    createInfo->HardcoreSelfFound = false;
    createInfo->TimerunningSeasonID = 0;

    // Initialize customizations (empty for bots - can be enhanced later)
    // Note: Customizations array is already default-initialized by CharacterCreateInfo constructor

    // Create Player object
    ::std::shared_ptr<Player> newChar(new Player(tempSession.get()), [](Player* ptr)
    {
        ptr->CleanupsBeforeDelete();
        delete ptr;
    });

    newChar->GetMotionMaster()->Initialize();

    // Generate new character GUID
    ObjectGuid::LowType guidLow = sObjectMgr->GetGenerator<HighGuid::Player>().Generate();

    // Call Player::Create
    if (!newChar->Create(guidLow, createInfo.get()))
    {
        outErrorMsg = "Player::Create failed (race/class problem or database issue)";
        TC_LOG_ERROR("module.playerbot", "BotCharacterCreator: Player::Create failed for bot '{}' (race: {}, class: {})",
            name, race, classId);
        return CreateResult::CREATION_FAILED;
    }

    // Set character as bot (skip cinematics)
    newChar->setCinematic(1);

    // Set first login flag
    newChar->SetAtLoginFlag(AT_LOGIN_FIRST);

    // POSITION VALIDATION FIX: Ensure bot has valid position before saving
    // Position defaults to (0,0,0) and IsPositionValid() returns TRUE for (0,0,0)
    // because it only checks coordinate bounds, not gameplay validity.
    if (newChar->GetPositionX() == 0.0f && newChar->GetPositionY() == 0.0f && newChar->GetPositionZ() == 0.0f)
    {
        TC_LOG_ERROR("module.playerbot",
            "BotCharacterCreator: POSITION BUG - Bot '{}' has (0,0,0) position after Create()! "
            "Race: {}, Class: {}. Fixing using playercreateinfo.",
            name, race, classId);

        // Get correct starting position from playercreateinfo
        PlayerInfo const* info = sObjectMgr->GetPlayerInfo(race, classId);
        if (info)
        {
            PlayerInfo::CreatePosition const& startPos = info->createPosition;
            newChar->Relocate(startPos.Loc);
            newChar->SetHomebind(startPos.Loc, newChar->GetAreaId());

            TC_LOG_INFO("module.playerbot",
                "BotCharacterCreator: Position fixed - Bot '{}' relocated to ({:.2f}, {:.2f}, {:.2f}) on map {}",
                name, startPos.Loc.GetPositionX(), startPos.Loc.GetPositionY(),
                startPos.Loc.GetPositionZ(), startPos.Loc.GetMapId());
        }
    }

    // NOTE: Specialization spells are NOT saved to database in modern WoW
    // They are learned dynamically from DB2 data on each login via LearnSpecializationSpells()
    // This is by design - see Player::LoadFromDB() line 18356

    // Prepare database transactions
    CharacterDatabaseTransaction charTransaction = CharacterDatabase.BeginTransaction();
    LoginDatabaseTransaction loginTransaction = LoginDatabase.BeginTransaction();

    try
    {
        // Save character to database
        newChar->SaveToDB(loginTransaction, charTransaction, true);

        // Update realm character count
        LoginDatabasePreparedStatement* loginStmt = LoginDatabase.GetPreparedStatement(LOGIN_REP_REALM_CHARACTERS);
        uint32 currentCount = 0;
        CanCreateCharacter(accountId, currentCount);
        loginStmt->setUInt32(0, currentCount + 1);
        loginStmt->setUInt32(1, accountId);
        loginStmt->setUInt32(2, sRealmList->GetCurrentRealmId().Realm);
        loginTransaction->Append(loginStmt);

        // Commit transactions synchronously (we need immediate result)
        CharacterDatabase.DirectCommitTransaction(charTransaction);
        LoginDatabase.DirectCommitTransaction(loginTransaction);

        // Add to character cache
        sCharacterCache->AddCharacterCacheEntry(
            newChar->GetGUID(),
            accountId,
            newChar->GetName(),
            newChar->GetNativeGender(),
            newChar->GetRace(),
            newChar->GetClass(),
            newChar->GetLevel(),
            false // Not deleted
        );

        // Success - return GUID
        outGuid = newChar->GetGUID();

        TC_LOG_INFO("module.playerbot", "BotCharacterCreator: Successfully created bot character '{}' (GUID: {}, Race: {}, Class: {}, Level: {})",
            name, outGuid.ToString(), race, classId, newChar->GetLevel());

        // Call script hook
        sScriptMgr->OnPlayerCreate(newChar.get());

        return CreateResult::SUCCESS;
    }
    catch (...)
    {
        outErrorMsg = "Database transaction failed during character creation";
        TC_LOG_ERROR("module.playerbot", "BotCharacterCreator: Database exception during bot creation for '{}'", name);
        return CreateResult::DATABASE_ERROR;
    }
}

} // namespace Playerbot
