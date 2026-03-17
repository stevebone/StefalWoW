/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifdef BUILD_PLAYERBOT

#include "PlayerbotModule.h"
#include "Core/PlayerbotSubsystemRegistry.h"
#include "Core/SubsystemAdapters.h"
#include "Config/PlayerbotConfig.h"
#include "Config/PlayerbotTradeConfig.h"
#include "Config/GuidedSetupHelper.h"
#include "Account/BotAccountMgr.h"
#include "Character/BotNameMgr.h"
#include "Character/BotCharacterDistribution.h"
#include "Database/PlayerbotDatabase.h"
#include "Database/PlayerbotCharacterDBInterface.h"
#include "Database/PlayerbotMigrationMgr.h"
#include "Lifecycle/BotSpawner.h"
#include "Lifecycle/Protection/BotProtectionRegistry.h"
#include "Lifecycle/Retirement/BotRetirementManager.h"
#include "Lifecycle/Prediction/BracketFlowPredictor.h"
#include "Lifecycle/Demand/PlayerActivityTracker.h"
#include "Lifecycle/Demand/DemandCalculator.h"
#include "Lifecycle/PopulationLifecycleController.h"

// Instance Bot Pool System (Hybrid Warm Pool + Elastic Overflow)
#include "Lifecycle/Instance/InstanceBotPool.h"
#include "Lifecycle/Instance/InstanceBotOrchestrator.h"
#include "Lifecycle/Instance/InstanceBotHooks.h"
#include "Lifecycle/Instance/ContentRequirements.h"
#include "Lifecycle/Instance/JITBotFactory.h"
#include "Lifecycle/Instance/BotTemplateRepository.h"
#include "Lifecycle/Instance/BotCloneEngine.h"
#include "Lifecycle/Instance/BotPostLoginConfigurator.h"
#include "Lifecycle/Instance/QueueStatePoller.h"
#include "Lifecycle/Instance/QueueShortageSubscriber.h"

// Enterprise-Grade Diagnostics System
#include "Core/Diagnostics/BotOperationTracker.h"

#include "Session/BotWorldSessionMgr.h"
#include "Session/BotPacketRelay.h"
#include "Chat/BotChatCommandHandler.h"
#include "Professions/ProfessionManager.h"
#include "Professions/ProfessionDatabase.h"
#include "Professions/ProfessionAuctionBridge.h"
#include "Professions/GatheringMaterialsBridge.h"
#include "Professions/AuctionMaterialsBridge.h"
#include "Banking/BankingManager.h"
#include "Quest/QuestHubDatabase.h"
#include "Travel/PortalDatabase.h"
#include "Equipment/BotGearFactory.h"
#include "AI/ClassAI/ClassBehaviorTreeRegistry.h"
#include "PlayerbotModuleAdapter.h"
#include "Update/ModuleUpdateManager.h"
#include "Group/GroupEvents.h"
#include "Network/PlayerbotPacketSniffer.h"

// Domain-Specific EventBus System
#include "Core/Events/GenericEventBus.h"
#include "Combat/CombatEvents.h"
#include "Loot/LootEvents.h"
#include "Quest/QuestEvents.h"
#include "Aura/AuraEvents.h"
#include "Cooldown/CooldownEvents.h"
#include "Cooldown/MajorCooldownTracker.h"
#include "Resource/ResourceEvents.h"
#include "Social/SocialEvents.h"
#include "Auction/AuctionEvents.h"
#include "NPC/NPCEvents.h"
#include "Instance/InstanceEvents.h"
#include "Professions/ProfessionEvents.h"
#include "Threading/BotActionManager.h"
#include "AI/Coordination/Battleground/Scripts/BGScriptInit.h"
#include "Log.h"
#include "GitRevision.h"
#include "GameTime.h"
#include <chrono>

// Module state - using inline static in header for DLL compatibility

bool PlayerbotModule::Initialize()
{
    TC_LOG_INFO("module.playerbot", "Initializing Playerbot Module...");

    // Run guided setup check FIRST - ensures config file exists
    // This will create a default config from .dist if missing
    if (!Playerbot::GuidedSetupHelper::CheckAndRunSetup())
    {
        _lastError = "Configuration setup failed - see logs for details";
        TC_LOG_ERROR("module.playerbot", "Playerbot Module: {}", _lastError);
        return false;
    }

    // Load configuration first
    if (!sPlayerbotConfig->Initialize())
    {
        _lastError = "Failed to load playerbot configuration";
        TC_LOG_ERROR("module.playerbot", "Playerbot Module: {}", _lastError);
        return false;
    }

    // Check if playerbot is enabled
    if (!sPlayerbotConfig->GetBool("Playerbot.Enable", false))
    {
        TC_LOG_INFO("module.playerbot", "Playerbot Module: Disabled in configuration");
        _initialized = true;
        _enabled = false;
        return true;
    }

    // Validate configuration
    if (!ValidateConfig())
    {
        _lastError = "Configuration validation failed";
        TC_LOG_ERROR("module.playerbot", "Playerbot Module: {}", _lastError);
        return false;
    }

    // Initialize logging
    InitializeLogging();

    // Load subsystem configurations
    TC_LOG_INFO("module.playerbot", "Loading Playerbot subsystem configurations...");
    Playerbot::PlayerbotTradeConfig::Load();
    TC_LOG_INFO("module.playerbot", "Playerbot subsystem configurations loaded");

    // Initialize Playerbot Database
    // CRITICAL: If this fails, the server MUST NOT start when Playerbot is enabled
    if (!InitializeDatabase())
    {
        // _lastError is already set by InitializeDatabase() with detailed information
        TC_LOG_ERROR("module.playerbot", "");
        TC_LOG_ERROR("module.playerbot", "  >>> SERVER STARTUP ABORTED <<<");
        TC_LOG_ERROR("module.playerbot", "");
        TC_LOG_ERROR("module.playerbot", "  Playerbot module failed to initialize due to database connection failure.");
        TC_LOG_ERROR("module.playerbot", "  Error: {}", _lastError);
        TC_LOG_ERROR("module.playerbot", "");
        return false;
    }

    // Initialize Character Database Interface
    if (!sPlayerbotCharDB->Initialize())
    {
        _lastError = "Failed to initialize Character Database Interface";
        TC_LOG_ERROR("module.playerbot", "Playerbot Module: {}", _lastError);
        return false;
    }

    // Initialize Migration Manager FIRST (before any database access)
    if (!PlayerbotMigrationMgr::instance()->Initialize())
    {
        _lastError = "Failed to initialize Migration Manager";
        return false;
    }

    // Apply pending migrations
    if (!PlayerbotMigrationMgr::instance()->ApplyMigrations())
    {
        _lastError = "Failed to apply database migrations";
        return false;
    }

    // Verify database version matches source code version
    // This ensures schema synchronization between builds
    PlayerbotMigrationMgr::instance()->CheckVersionMismatch();

    // Initialize all sub-managers
    if (!InitializeManagers())
    {
        return false;
    }

    // Initialize BG Scripts (forces linker to include script object files)
    Playerbot::Coordination::Battleground::InitializeBGScripts();

    // ==========================================================================

    // NOTE: Do NOT register with ModuleUpdateManager here!
    // PlayerbotModuleAdapter already registers with ModuleManager, which calls OnModuleUpdate
    // → OnWorldUpdate. Registering here would cause DOUBLE updates per tick, leading to
    // FreezeDetector crashes (60s timeout exceeded due to 35s × 2 = 70s max wait).
    // See: World.cpp lines 2327 (ModuleManager::CallOnUpdate) and 2334 (sModuleUpdateManager->Update)

    _initialized = true;
    _enabled = true;


    return true;
}

void PlayerbotModule::Shutdown()
{
    if (!_initialized)
        return;

    TC_LOG_INFO("module.playerbot", "Shutting down Playerbot Module...");

    // Shutdown all registered subsystems in shutdownOrder
    sPlayerbotSubsystemRegistry->ShutdownAll();

    // Unregister hooks (NOT a subsystem, stays here)
    UnregisterHooks();

    // Unregister from ModuleUpdateManager
    sModuleUpdateManager->UnregisterModule("playerbot");
    TC_LOG_DEBUG("module.playerbot", "Unregistered playerbot from ModuleUpdateManager");

    if (_enabled)
    {
        // Shutdown Playerbot Database (AFTER all subsystems)
        TC_LOG_INFO("module.playerbot", "Shutting down Playerbot Database...");
        ShutdownDatabase();

        // Then shutdown Character Database Interface after operations complete
        TC_LOG_INFO("module.playerbot", "Shutting down Character Database Interface...");
        sPlayerbotCharDB->Shutdown();
    }

    // Mark as shutdown
    _initialized = false;
    _enabled = false;

    TC_LOG_INFO("module.playerbot", "Playerbot Module: Shutdown complete");
}

bool PlayerbotModule::IsEnabled()
{
    return _initialized && _enabled;
}

std::string PlayerbotModule::GetVersion()
{
    return Trinity::StringFormat("{}.{}.{}", MODULE_VERSION_MAJOR, MODULE_VERSION_MINOR, MODULE_VERSION_PATCH);
}

std::string PlayerbotModule::GetBuildInfo()
{
    return Trinity::StringFormat("Playerbot Module {} (Built: {} {})",
        GetVersion(), __DATE__, __TIME__);
}

void PlayerbotModule::RegisterHooks()
{
    // Register with ModuleManager for reliable lifecycle management
    Playerbot::PlayerbotModuleAdapter::RegisterWithModuleManager();
}

void PlayerbotModule::UnregisterHooks()
{
    // TrinityCore ScriptMgr does not support script deregistration at runtime.
    // Registered hooks persist until server shutdown, which is the intended behavior.
}

void PlayerbotModule::OnWorldUpdate(uint32 diff)
{
    if (!_enabled || !_initialized)
        return;

    // CRITICAL SAFETY: Wrap entire update in try-catch to prevent crashes
    try
    {
        // One-time trigger to complete login for existing sessions
        static bool loginTriggered = false;
        static uint32 totalTime = 0;
        totalTime += diff;

        if (!loginTriggered && totalTime > 5000) // Wait 5 seconds after startup
        {
            TC_LOG_INFO("module.playerbot", " OnWorldUpdate: Auto-triggering character logins for existing sessions");
            TriggerBotCharacterLogins();
            loginTriggered = true;
        }

        // Update all registered subsystems with automatic profiling
        sPlayerbotSubsystemRegistry->UpdateAll(diff);
    }
    catch (std::exception const& ex)
    {
        TC_LOG_ERROR("module.playerbot", "CRITICAL EXCEPTION in PlayerbotModule::OnWorldUpdate: {}", ex.what());
        TC_LOG_ERROR("module.playerbot", "Disabling playerbot to prevent further crashes");
        _enabled = false;
    }
    catch (...)
    {
        TC_LOG_ERROR("module.playerbot", "CRITICAL UNKNOWN EXCEPTION in PlayerbotModule::OnWorldUpdate");
        TC_LOG_ERROR("module.playerbot", "Disabling playerbot to prevent further crashes");
        _enabled = false;
    }
}

bool PlayerbotModule::ValidateConfig()
{
    // Basic configuration validation
    if (!sPlayerbotConfig)
    {
        _lastError = "Configuration system not initialized";
        return false;
    }

    // Validate bot count limits
    uint32 maxBots = sPlayerbotConfig->GetInt("Playerbot.MaxBotsPerAccount", 10);
    if (maxBots < 1 || maxBots > 50)
    {
        _lastError = Trinity::StringFormat("Playerbot.MaxBotsPerAccount invalid ({}), must be between 1-50", maxBots);
        TC_LOG_WARN("module.playerbot", "Playerbot Module: {}", _lastError);
        return false;
    }

    // Validate character limit per account - WICHTIG: MAX 10!
    uint32 maxChars = sPlayerbotConfig->GetInt("Playerbot.MaxCharactersPerAccount", 10);
    if (maxChars < 1 || maxChars > 10)
    {
        _lastError = Trinity::StringFormat("Playerbot.MaxCharactersPerAccount invalid ({}), must be between 1-10", maxChars);
        TC_LOG_WARN("module.playerbot", "Playerbot Module: {}", _lastError);
        return false;
    }

    // Validate update intervals
    uint32 updateMs = sPlayerbotConfig->GetInt("Playerbot.UpdateInterval", 1000);
    if (updateMs < 100)
    {
        _lastError = "Playerbot.UpdateInterval too low (minimum 100ms)";
        TC_LOG_WARN("module.playerbot", "Playerbot Module: {}", _lastError);
        return false;
    }

    return true;
}

void PlayerbotModule::InitializeLogging()
{
    // Check if sPlayerbotConfig is valid
    if (!sPlayerbotConfig)
    {
        return;
    }

    sPlayerbotConfig->InitializeLogging();
}

bool PlayerbotModule::InitializeDatabase()
{
    // Get individual database settings from configuration
    std::string host = sPlayerbotConfig->GetString("Playerbot.Database.Host", "127.0.0.1");
    uint32 port = sPlayerbotConfig->GetInt("Playerbot.Database.Port", 3306);
    std::string user = sPlayerbotConfig->GetString("Playerbot.Database.User", "trinity");
    std::string password = sPlayerbotConfig->GetString("Playerbot.Database.Password", "trinity");
    std::string database = sPlayerbotConfig->GetString("Playerbot.Database.Name", "characters");

    // Construct connection string in format: hostname;port;username;password;database
    std::string dbString = Trinity::StringFormat("{};{};{};{};{}", host, port, user, password, database);

    TC_LOG_INFO("module.playerbot", "Playerbot Database: Connecting to {}:{}/{}", host, port, database);

    // Initialize database connection using our custom manager
    if (!sPlayerbotDatabase->Initialize(dbString))
    {
        // ============================================================================
        // CRITICAL DATABASE CONNECTION FAILURE - BLOCK SERVER STARTUP
        // ============================================================================
        TC_LOG_ERROR("module.playerbot", "");
        TC_LOG_ERROR("module.playerbot", "================================================================================");
        TC_LOG_ERROR("module.playerbot", "  PLAYERBOT DATABASE CONNECTION FAILED - SERVER STARTUP BLOCKED");
        TC_LOG_ERROR("module.playerbot", "================================================================================");
        TC_LOG_ERROR("module.playerbot", "");
        TC_LOG_ERROR("module.playerbot", "  Playerbot is ENABLED but cannot connect to its database.");
        TC_LOG_ERROR("module.playerbot", "  The server cannot start safely without a working database connection.");
        TC_LOG_ERROR("module.playerbot", "");
        TC_LOG_ERROR("module.playerbot", "  Current Configuration:");
        TC_LOG_ERROR("module.playerbot", "    Host:     {}", host);
        TC_LOG_ERROR("module.playerbot", "    Port:     {}", port);
        TC_LOG_ERROR("module.playerbot", "    User:     {}", user);
        TC_LOG_ERROR("module.playerbot", "    Database: {}", database);
        TC_LOG_ERROR("module.playerbot", "");
        TC_LOG_ERROR("module.playerbot", "  Possible Causes:");
        TC_LOG_ERROR("module.playerbot", "    1. MySQL server is not running");
        TC_LOG_ERROR("module.playerbot", "    2. Wrong hostname or port in configuration");
        TC_LOG_ERROR("module.playerbot", "    3. Invalid username or password");
        TC_LOG_ERROR("module.playerbot", "    4. Database '{}' does not exist", database);
        TC_LOG_ERROR("module.playerbot", "    5. User '{}' has no access to database '{}'", user, database);
        TC_LOG_ERROR("module.playerbot", "    6. Firewall blocking connection to port {}", port);
        TC_LOG_ERROR("module.playerbot", "");
        TC_LOG_ERROR("module.playerbot", "  Solutions:");
        TC_LOG_ERROR("module.playerbot", "    - Check worldserver.conf for Playerbot.Database.* settings");
        TC_LOG_ERROR("module.playerbot", "    - Verify MySQL server is running: mysql -u {} -p -h {} -P {}", user, host, port);
        TC_LOG_ERROR("module.playerbot", "    - Create database if missing: CREATE DATABASE {};", database);
        TC_LOG_ERROR("module.playerbot", "    - Or disable Playerbot: set Playerbot.Enable = 0");
        TC_LOG_ERROR("module.playerbot", "");
        TC_LOG_ERROR("module.playerbot", "================================================================================");
        TC_LOG_ERROR("module.playerbot", "");

        _lastError = Trinity::StringFormat(
            "CRITICAL: Playerbot database connection failed! "
            "Cannot connect to {}:{}/{} as user '{}'. "
            "Check your Playerbot.Database.* configuration or disable Playerbot (Playerbot.Enable = 0)",
            host, port, database, user);

        return false;
    }

    TC_LOG_INFO("module.playerbot", "Playerbot Database: Successfully connected to {}:{}/{}", host, port, database);

    // Validate database schema
    TC_LOG_INFO("module.playerbot", "Validating Playerbot Database Schema...");
    if (!sPlayerbotDatabase->ValidateSchema())
    {
        TC_LOG_WARN("module.playerbot", "");
        TC_LOG_WARN("module.playerbot", "================================================================================");
        TC_LOG_WARN("module.playerbot", "  PLAYERBOT DATABASE SCHEMA WARNING");
        TC_LOG_WARN("module.playerbot", "================================================================================");
        TC_LOG_WARN("module.playerbot", "  Database schema validation failed - some tables may be missing or outdated.");
        TC_LOG_WARN("module.playerbot", "  Playerbot will continue but some features may not work correctly.");
        TC_LOG_WARN("module.playerbot", "");
        TC_LOG_WARN("module.playerbot", "  Solution: Run the database migrations in sql/playerbot/ directory");
        TC_LOG_WARN("module.playerbot", "================================================================================");
        TC_LOG_WARN("module.playerbot", "");
    }

    return true;
}

void PlayerbotModule::TriggerBotCharacterLogins()
{
    if (!_enabled || !_initialized)
    {
        TC_LOG_WARN("module.playerbot", "TriggerBotCharacterLogins: Module not enabled or initialized");
        return;
    }

    TC_LOG_INFO("module.playerbot", " TriggerBotCharacterLogins: Manually triggering character logins for existing sessions");

    // Call the BotWorldSessionMgr method to trigger native logins
    Playerbot::sBotWorldSessionMgr->TriggerCharacterLoginForAllSessions();

    TC_LOG_INFO("module.playerbot", " TriggerBotCharacterLogins: Complete");
}

bool PlayerbotModule::InitializeManagers()
{
    // Register hooks with TrinityCore (must happen before subsystem init)
    RegisterHooks();

    // Register all subsystem adapters with the registry
    Playerbot::RegisterAllSubsystems();

    // Initialize all subsystems in order, with formatted startup banner
    return sPlayerbotSubsystemRegistry->InitializeAll(GetVersion());
}

void PlayerbotModule::ShutdownDatabase()
{
    TC_LOG_DEBUG("module.playerbot", "Closing Playerbot Database connection");
    sPlayerbotDatabase->Close();
    TC_LOG_DEBUG("module.playerbot", "Playerbot Database connection closed");
}

#endif // BUILD_PLAYERBOT
