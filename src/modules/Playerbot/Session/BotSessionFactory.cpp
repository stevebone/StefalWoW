/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "BotSessionFactory.h"
#include "BotSession.h"
#include "../AI/BotAI.h"  // P1 FIX: Required for unique_ptr<BotAI> in BotSession.h
#include "Lifecycle/BotSpawner.h"
#include "Logging/Log.h"
#include "WorldSession.h"
#include "AccountMgr.h"
#include "CharacterCache.h"
#include "Player.h"
#include <chrono>

namespace Playerbot
{

BotSessionFactory::BotSessionFactory()
{
}

bool BotSessionFactory::Initialize()
{
    TC_LOG_INFO("module.playerbot.session.factory",
        "Initializing BotSessionFactory for streamlined session creation");

    LoadDefaultTemplates();
    UpdateConfigurationCache();
    ResetStats();

    TC_LOG_INFO("module.playerbot.session.factory",
        "BotSessionFactory initialized - Templates: {}, Cache valid: {}",
        _sessionTemplates.size(), IsCacheValid());

    return true;
}

void BotSessionFactory::Shutdown()
{
    TC_LOG_INFO("module.playerbot.session.factory", "Shutting down BotSessionFactory");

    auto const& stats = GetStats();
    TC_LOG_INFO("module.playerbot.session.factory",
        "Final Factory Statistics - Created: {}, Success Rate: {:.1f}%, Avg Time: {}μs",
        stats.sessionsCreated.load(), stats.GetSuccessRate(), stats.avgCreationTimeUs.load());

    // Clear templates
    {
        ::std::lock_guard lock(_templateMutex);
        _sessionTemplates.clear();
    }

    // Clear cache
    {
        ::std::lock_guard lock(_cacheMutex);
        _configCache.classConfigurations.clear();
        _configCache.zoneConfigurations.clear();
        _configCache.isValid = false;
    }
}

// === SESSION CREATION ===

::std::shared_ptr<BotSession> BotSessionFactory::CreateBotSession(ObjectGuid characterGuid, SpawnRequest const& request)
{
    auto start = ::std::chrono::high_resolution_clock::now();

    // Get account ID from character
    uint32 accountId = 0;
    if (CharacterCacheEntry const* characterInfo = sCharacterCache->GetCharacterCacheByGuid(characterGuid))
        accountId = characterInfo->AccountId;

    if (accountId == 0)
    {
        HandleCreationError("Invalid character GUID or account not found", characterGuid);
        return nullptr;
    }

    return CreateBotSession(accountId, characterGuid);
}

::std::shared_ptr<BotSession> BotSessionFactory::CreateBotSession(uint32 accountId, ObjectGuid characterGuid)
{
    auto start = ::std::chrono::high_resolution_clock::now();

    try
    {
        // Validate inputs
    if (!ValidateAccountAccess(accountId) || !ValidateCharacterData(characterGuid))
        {
            HandleCreationError("Account or character validation failed", characterGuid);
            return nullptr;
        }

        // Create the session
        auto session = CreateSessionInternal(accountId, characterGuid);
        if (!session)
        {
            HandleCreationError("Session creation failed", characterGuid);
            return nullptr;
        }

        // Validate the created session
    if (!ValidateSession(session))
        {
            HandleCreationError("Session validation failed", characterGuid);
            return nullptr;
        }

        // Record successful creation
        auto end = ::std::chrono::high_resolution_clock::now();
        auto duration = ::std::chrono::duration_cast<::std::chrono::microseconds>(end - start);
        RecordCreation(duration.count(), true);

        TC_LOG_DEBUG("module.playerbot.session.factory",
            "Successfully created bot session for character {} (account {})",
            characterGuid.ToString(), accountId);

        return session;
    }
    catch (::std::exception const& ex)
    {
        HandleCreationError(ex.what(), characterGuid);
        auto end = ::std::chrono::high_resolution_clock::now();
        auto duration = ::std::chrono::duration_cast<::std::chrono::microseconds>(end - start);
        RecordCreation(duration.count(), false);
        return nullptr;
    }
}

::std::vector<::std::shared_ptr<BotSession>> BotSessionFactory::CreateBotSessions(
    ::std::vector<ObjectGuid> const& characterGuids,
    SpawnRequest const& baseRequest)
{
    ::std::vector<::std::shared_ptr<BotSession>> sessions;
    sessions.reserve(characterGuids.size());

    for (ObjectGuid const& characterGuid : characterGuids)
    {
        auto session = CreateBotSession(characterGuid, baseRequest);
        if (session)
        {
            sessions.push_back(session);
        }
    }

    TC_LOG_DEBUG("module.playerbot.session.factory",
        "Batch created {}/{} bot sessions", sessions.size(), characterGuids.size());

    return sessions;
}

// === SESSION CONFIGURATION ===

bool BotSessionFactory::ConfigureSession(::std::shared_ptr<BotSession> session, SpawnRequest const& request)
{
    if (!session)
        return false;

    try
    {
        ApplyBaseConfiguration(session, request);

        // Apply specific configurations based on character data
    if (Player* player = session->GetPlayer())
        {
            ApplyClassSpecificConfiguration(session, player->GetClass());
            ApplyLevelConfiguration(session, player->GetLevel());
        }

        ApplyZoneConfiguration(session, request.zoneId);

        return true;
    }
    catch (::std::exception const& ex)
    {
        TC_LOG_ERROR("module.playerbot.session.factory",
            "Failed to configure session: {}", ex.what());
        return false;
    }
}

bool BotSessionFactory::ValidateSession(::std::shared_ptr<BotSession> session) const
{
    return session &&
           session->GetAccountId() != 0 &&
           ValidateSessionConfiguration(session);
}

// === SESSION TEMPLATES ===

void BotSessionFactory::RegisterSessionTemplate(::std::string const& templateName, SpawnRequest const& templateRequest)
{
    ::std::lock_guard lock(_templateMutex);

    if (_sessionTemplates.size() >= MAX_TEMPLATES)
    {
        TC_LOG_WARN("module.playerbot.session.factory",
            "Cannot register template '{}' - maximum templates ({}) reached", templateName, MAX_TEMPLATES);
        return;
    }

    SessionTemplate& sessionTemplate = _sessionTemplates[templateName];
    sessionTemplate.name = templateName;
    sessionTemplate.baseRequest = templateRequest;
    sessionTemplate.usageCount = 0;

    TC_LOG_DEBUG("module.playerbot.session.factory",
        "Registered session template '{}'", templateName);
}

::std::shared_ptr<BotSession> BotSessionFactory::CreateFromTemplate(::std::string const& templateName, ObjectGuid characterGuid)
{
    SessionTemplate const* sessionTemplate = GetTemplate(templateName);
    if (!sessionTemplate)
    {
        TC_LOG_WARN("module.playerbot.session.factory",
            "Template '{}' not found", templateName);
        return nullptr;
    }

    auto session = CreateBotSession(characterGuid, sessionTemplate->baseRequest);
    if (session)
    {
        RecordTemplateUsage(templateName);
        TC_LOG_DEBUG("module.playerbot.session.factory",
            "Created session from template '{}'", templateName);
    }

    return session;
}

// === PRIVATE IMPLEMENTATION ===

::std::shared_ptr<BotSession> BotSessionFactory::CreateSessionInternal(uint32 accountId, ObjectGuid characterGuid)
{
    // Create the BotSession instance using factory method
    auto session = BotSession::Create(accountId);

    // Basic session initialization would go here
    // This is a simplified implementation - in reality would need:
    // - Socket simulation setup
    // - Character loading
    // - World session initialization
    // - Bot AI attachment

    return session;
}

bool BotSessionFactory::InitializeSessionComponents(::std::shared_ptr<BotSession> session, SpawnRequest const& request)
{
    // Initialize session components based on spawn request
    // This would include:
    // - AI initialization
    // - Packet handler setup
    // - Character state loading
    // - Equipment and spell initialization

    return session != nullptr;
}

void BotSessionFactory::ApplyBaseConfiguration(::std::shared_ptr<BotSession> session, SpawnRequest const& request)
{
    // Apply base configuration from spawn request
    // This would set:
    // - Basic bot behavior parameters
    // - Movement and combat settings
    // - Social interaction settings
}

void BotSessionFactory::ApplyClassSpecificConfiguration(::std::shared_ptr<BotSession> session, uint8 playerClass)
{
    // Apply class-specific AI and behavior configuration
    ::std::lock_guard lock(_cacheMutex);

    auto it = _configCache.classConfigurations.find(playerClass);
    if (it != _configCache.classConfigurations.end())
    {
        // Apply cached class configuration
        TC_LOG_DEBUG("module.playerbot.session.factory",
            "Applied class {} configuration", playerClass);
    }
}

void BotSessionFactory::ApplyLevelConfiguration(::std::shared_ptr<BotSession> session, uint8 level)
{
    // Apply level-appropriate behavior and difficulty settings
    // Adjust AI aggressiveness, spell usage, etc.
}

void BotSessionFactory::ApplyZoneConfiguration(::std::shared_ptr<BotSession> session, uint32 zoneId)
{
    // Apply zone-specific behavior settings
    ::std::lock_guard lock(_cacheMutex);

    auto it = _configCache.zoneConfigurations.find(zoneId);
    if (it != _configCache.zoneConfigurations.end())
    {
        // Apply cached zone configuration
        TC_LOG_DEBUG("module.playerbot.session.factory",
            "Applied zone {} configuration", zoneId);
    }
}

// === VALIDATION ===

bool BotSessionFactory::ValidateAccountAccess(uint32 accountId) const
{
    ::std::string accountName;
    if (!AccountMgr::GetName(accountId, accountName))
        return false;

    return AccountMgr::GetId(accountName) == accountId;
}

bool BotSessionFactory::ValidateCharacterData(ObjectGuid characterGuid) const
{
    return sCharacterCache->GetCharacterCacheByGuid(characterGuid) != nullptr;
}

bool BotSessionFactory::ValidateSessionConfiguration(::std::shared_ptr<BotSession> session) const
{
    // Validate that the session is properly configured
    return session->GetAccountId() != 0;
}

// === TEMPLATES ===

void BotSessionFactory::LoadDefaultTemplates()
{
    ::std::lock_guard lock(_templateMutex);

    // Create default templates for common bot types
    SpawnRequest defaultRequest;
    defaultRequest.zoneId = 0;  // Any zone
    defaultRequest.minLevel = 1;
    defaultRequest.maxLevel = 80;

    SessionTemplate defaultTemplate;
    defaultTemplate.name = "default";
    defaultTemplate.baseRequest = defaultRequest;
    defaultTemplate.usageCount = 0;

    _sessionTemplates["default"] = defaultTemplate;

    TC_LOG_DEBUG("module.playerbot.session.factory", "Loaded {} default templates", 1);
}

BotSessionFactory::SessionTemplate const* BotSessionFactory::GetTemplate(::std::string const& templateName) const
{
    ::std::lock_guard lock(_templateMutex);

    auto it = _sessionTemplates.find(templateName);
    return it != _sessionTemplates.end() ? &it->second : nullptr;
}

// === CACHING ===

void BotSessionFactory::UpdateConfigurationCache()
{
    ::std::lock_guard lock(_cacheMutex);

    // Update class configurations
    _configCache.classConfigurations.clear();
    for (uint8 classId = 1; classId <= 12; ++classId)  // All player classes
    {
        _configCache.classConfigurations[classId] = "default_class_config";
    }

    // Update zone configurations
    _configCache.zoneConfigurations.clear();
    // Would populate with actual zone configurations

    _configCache.lastUpdate = ::std::chrono::steady_clock::now();
    _configCache.isValid = true;

    TC_LOG_DEBUG("module.playerbot.session.factory", "Updated configuration cache");
}

bool BotSessionFactory::IsCacheValid() const
{
    ::std::lock_guard lock(_cacheMutex);

    if (!_configCache.isValid)
        return false;

    auto now = ::std::chrono::steady_clock::now();
    auto elapsed = ::std::chrono::duration_cast<::std::chrono::milliseconds>(now - _configCache.lastUpdate);

    return elapsed.count() < CACHE_VALIDITY_MS;
}

// === PERFORMANCE TRACKING ===

void BotSessionFactory::RecordCreation(uint64 durationMicroseconds, bool success)
{
    if (success)
    {
        _stats.sessionsCreated.fetch_add(1);

        // Update average creation time
        uint64 currentAvg = _stats.avgCreationTimeUs.load();
        uint32 count = _stats.sessionsCreated.load();
        uint64 newAvg = (currentAvg * (count - 1) + durationMicroseconds) / count;
        _stats.avgCreationTimeUs.store(newAvg);
    }
    else
    {
        _stats.creationFailures.fetch_add(1);
    }
}

void BotSessionFactory::RecordTemplateUsage(::std::string const& templateName)
{
    _stats.templatesUsed.fetch_add(1);

    ::std::lock_guard lock(_templateMutex);
    auto it = _sessionTemplates.find(templateName);
    if (it != _sessionTemplates.end())
    {
        it->second.usageCount++;
    }
}

void BotSessionFactory::ResetStats()
{
    _stats.sessionsCreated.store(0);
    _stats.creationFailures.store(0);
    _stats.configurationFailures.store(0);
    _stats.avgCreationTimeUs.store(0);
    _stats.templatesUsed.store(0);
}

// === ERROR HANDLING ===

void BotSessionFactory::HandleCreationError(::std::string const& error, ObjectGuid characterGuid)
{
    TC_LOG_ERROR("module.playerbot.session.factory",
        "Session creation error for character {}: {}", characterGuid.ToString(), error);

    _stats.creationFailures.fetch_add(1);
}

::std::shared_ptr<BotSession> BotSessionFactory::CreateFallbackSession(ObjectGuid characterGuid)
{
    // Create a minimal session for error recovery
    // This would be a simplified session with minimal functionality
    return nullptr; // Simplified for now
}

} // namespace Playerbot