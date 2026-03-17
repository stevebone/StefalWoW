/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Define.h"
#include "Threading/LockHierarchy.h"
#include "ObjectGuid.h"
#include "Lifecycle/SpawnRequest.h"
#include <memory>
#include <atomic>
#include <mutex>
#include <unordered_map>

namespace Playerbot
{

// Forward declarations
class BotSession;

/**
 * @class BotSessionFactory
 * @brief Factory for creating and configuring bot sessions
 *
 * SINGLE RESPONSIBILITY: Handles all session creation logic extracted
 * from the monolithic BotSpawner class.
 *
 * Responsibilities:
 * - Create properly configured BotSession instances
 * - Apply session configuration based on spawn requests
 * - Handle session validation and error recovery
 * - Manage session lifecycle hooks
 * - Provide session templates and presets
 *
 * Performance Features:
 * - Session template caching
 * - Lazy initialization of session components
 * - Configuration validation caching
 * - Session creation metrics tracking
 */
class TC_GAME_API BotSessionFactory
{
public:
    BotSessionFactory();
    ~BotSessionFactory() = default;

    // Lifecycle
    bool Initialize();
    void Shutdown();

    // === SESSION CREATION ===
    ::std::shared_ptr<BotSession> CreateBotSession(ObjectGuid characterGuid, SpawnRequest const& request);
    ::std::shared_ptr<BotSession> CreateBotSession(uint32 accountId, ObjectGuid characterGuid);

    // === BATCH CREATION ===
    ::std::vector<::std::shared_ptr<BotSession>> CreateBotSessions(
        ::std::vector<ObjectGuid> const& characterGuids,
        SpawnRequest const& baseRequest);

    // === SESSION CONFIGURATION ===
    bool ConfigureSession(::std::shared_ptr<BotSession> session, SpawnRequest const& request);
    bool ValidateSession(::std::shared_ptr<BotSession> session) const;

    // === SESSION TEMPLATES ===
    void RegisterSessionTemplate(::std::string const& templateName, SpawnRequest const& templateRequest);
    ::std::shared_ptr<BotSession> CreateFromTemplate(::std::string const& templateName, ObjectGuid characterGuid);

    // === PERFORMANCE METRICS ===
    struct FactoryStats
    {
        ::std::atomic<uint32> sessionsCreated{0};
        ::std::atomic<uint32> creationFailures{0};
        ::std::atomic<uint32> configurationFailures{0};
        ::std::atomic<uint64> avgCreationTimeUs{0};
        ::std::atomic<uint32> templatesUsed{0};

        // Delete copy constructor and assignment operator for atomic members
        FactoryStats() = default;
        FactoryStats(FactoryStats const&) = delete;
        FactoryStats& operator=(FactoryStats const&) = delete;

        float GetSuccessRate() const
        {
            uint32 total = sessionsCreated.load() + creationFailures.load();
            return total > 0 ? static_cast<float>(sessionsCreated.load()) / total * 100.0f : 100.0f;
        }
    };

    FactoryStats const& GetStats() const { return _stats; }
    void ResetStats();

private:
    // === SESSION CREATION IMPLEMENTATION ===
    ::std::shared_ptr<BotSession> CreateSessionInternal(uint32 accountId, ObjectGuid characterGuid);
    bool InitializeSessionComponents(::std::shared_ptr<BotSession> session, SpawnRequest const& request);

    // === CONFIGURATION APPLICATION ===
    void ApplyBaseConfiguration(::std::shared_ptr<BotSession> session, SpawnRequest const& request);
    void ApplyClassSpecificConfiguration(::std::shared_ptr<BotSession> session, uint8 playerClass);
    void ApplyLevelConfiguration(::std::shared_ptr<BotSession> session, uint8 level);
    void ApplyZoneConfiguration(::std::shared_ptr<BotSession> session, uint32 zoneId);

    // === SESSION VALIDATION ===
    bool ValidateAccountAccess(uint32 accountId) const;
    bool ValidateCharacterData(ObjectGuid characterGuid) const;
    bool ValidateSessionConfiguration(::std::shared_ptr<BotSession> session) const;

    // === SESSION TEMPLATES ===
    struct SessionTemplate
    {
        ::std::string name;
        SpawnRequest baseRequest;
        ::std::unordered_map<::std::string, ::std::string> configOverrides;
        uint32 usageCount = 0;
    };

    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::SESSION_MANAGER> _templateMutex;
    ::std::unordered_map<::std::string, SessionTemplate> _sessionTemplates;

    void LoadDefaultTemplates();
    SessionTemplate const* GetTemplate(::std::string const& templateName) const;

    // === CACHING ===
    struct ConfigurationCache
    {
        ::std::unordered_map<uint8, ::std::string> classConfigurations; // class -> config
        ::std::unordered_map<uint32, ::std::string> zoneConfigurations; // zoneId -> config
        ::std::chrono::steady_clock::time_point lastUpdate;
        bool isValid = false;
    };

    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::SESSION_MANAGER> _cacheMutex;
    mutable ConfigurationCache _configCache;

    void UpdateConfigurationCache();
    bool IsCacheValid() const;

    // === PERFORMANCE TRACKING ===
    mutable FactoryStats _stats;
    void RecordCreation(uint64 durationMicroseconds, bool success);
    void RecordTemplateUsage(::std::string const& templateName);

    // === ERROR HANDLING ===
    void HandleCreationError(::std::string const& error, ObjectGuid characterGuid);
    ::std::shared_ptr<BotSession> CreateFallbackSession(ObjectGuid characterGuid);

    // === CONFIGURATION ===
    static constexpr uint32 CACHE_VALIDITY_MS = 60000;    // 1 minute
    static constexpr uint32 MAX_CREATION_TIME_MS = 1000;  // 1 second
    static constexpr uint32 MAX_TEMPLATES = 100;          // Template limit

    // Non-copyable
    BotSessionFactory(BotSessionFactory const&) = delete;
    BotSessionFactory& operator=(BotSessionFactory const&) = delete;
};

} // namespace Playerbot