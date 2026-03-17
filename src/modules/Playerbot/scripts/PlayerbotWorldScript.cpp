/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "PlayerbotWorldScript.h"
#include "ScriptMgr.h"
#include "World.h"
#include "GameTime.h"
#include "Log.h"
#include "Timer.h"
#include "Config/PlayerbotConfig.h"
#include "Lifecycle/BotSpawner.h"
#include "Session/BotWorldSessionMgr.h"
#include "Lifecycle/BotLifecycleMgr.h"
#include "Character/BotLevelManager.h"
#include "Core/PlayerBotHooks.h"
#include "Dungeon/DungeonScriptLoader.h"
// INTEGRATION REQUIRED: Re-enable when API compatibility is fixed
// Blocked by TrinityCore 12.0 API changes - see TODO at line 144 for details
// #include "Companion/MountManager.h"
// #include "Companion/BattlePetManager.h"

PlayerbotWorldScript::PlayerbotWorldScript() : WorldScript("PlayerbotWorldScript")
{
    TC_LOG_INFO("module.playerbot.script", "PlayerbotWorldScript constructor called - script registered");
}

void PlayerbotWorldScript::OnUpdate(uint32 diff)
{
    static bool initialized = false;
    static uint32 initRetryCount = 0;
    static uint32 updatesSinceModuleReady = 0;

    // Handle deferred initialization - check if Playerbot module is ready
    if (!initialized)
    {
        if (!IsPlayerbotEnabled())
        {
            // Module not ready yet, retry every 100 updates
    if (++initRetryCount % 100 == 0)
            {
                TC_LOG_DEBUG("module.playerbot.script", "PlayerbotWorldScript: Waiting for Playerbot module initialization (attempt {})", initRetryCount);
            }
            return;
        }

        // Module is now ready, but wait for database to be fully stable
        // Give it 100 update cycles (~10 seconds) before attempting database queries
    if (++updatesSinceModuleReady < 100)
        {
            if (updatesSinceModuleReady == 1)
            {
                TC_LOG_INFO("module.playerbot.script", "PlayerbotWorldScript: Playerbot module initialized - waiting for database stability...");
            }
            return;
        }

        // Database should be stable now
        // NOTE: BotGearFactory is initialized in PlayerbotModule::Initialize() (before world starts)
        initialized = true;

        // Initialize Automated World Population System
        TC_LOG_INFO("module.playerbot.script", "PlayerbotWorldScript: Initializing Automated World Population System...");
        if (sBotLevelManager->Initialize())
        {
            TC_LOG_INFO("module.playerbot.script", "Automated World Population System initialized successfully");

            // ================================================================
            // CRITICAL FIX: Trigger initial level redistribution at startup
            // ================================================================
            // Existing bots in the database may not follow the level distribution.
            // This ensures all bots are rebalanced to the configured distribution
            // (5% Starting, 15% Chromie Time, 20% Dragonflight, 60% TWW).
            // ================================================================
            TC_LOG_INFO("module.playerbot.script", "PlayerbotWorldScript: Triggering initial level distribution rebalance...");
            sBotLevelManager->RebalanceDistribution();
        }
        else
        {
            TC_LOG_ERROR("module.playerbot.script", "Automated World Population System failed to initialize");
        }

        // Initialize performance tracking
        _lastMetricUpdate = GameTime::GetGameTimeMS();
        _lastRebalanceCheck = GameTime::GetGameTimeMS();  // Initialize rebalance timer
        _totalUpdateTime = 0;
        _updateCount = 0;
    }

    uint32 startTime = GameTime::GetGameTimeMS();

    try
    {
        UpdateBotSystems(diff);
    }
    catch (std::exception const& e)
    {
        TC_LOG_ERROR("module.playerbot.script",
            "PlayerbotWorldScript::OnUpdate: Exception caught: {}", e.what());
    }

    uint32 updateTime = GetMSTimeDiffToNow(startTime);
    UpdateMetrics(updateTime);

    // Performance warning if update takes too long
    if (updateTime > 100) // 100ms threshold
    {
        TC_LOG_WARN("module.playerbot.script",
            "PlayerbotWorldScript::OnUpdate: Slow update detected - {}ms", updateTime);
    }
}

void PlayerbotWorldScript::OnConfigLoad(bool reload)
{
    if (!IsPlayerbotEnabled())
        return;

    TC_LOG_INFO("module.playerbot.script",
        "PlayerbotWorldScript::OnConfigLoad: {} configuration",
        reload ? "Reloading" : "Loading");

    if (reload)
    {
        // Handle dynamic configuration changes
        try
        {
            // Reload BotSpawner configuration
            Playerbot::sBotSpawner->LoadConfig();

            // Reload other module configurations as needed
            // Note: Full module restart may be required for some settings

            TC_LOG_INFO("module.playerbot.script", "Playerbot configuration reloaded successfully");
        }
        catch (std::exception const& e)
        {
            TC_LOG_ERROR("module.playerbot.script",
                "PlayerbotWorldScript::OnConfigLoad: Failed to reload configuration: {}", e.what());
        }
    }
}

void PlayerbotWorldScript::OnStartup()
{
    TC_LOG_INFO("module.playerbot.script", "PlayerbotWorldScript::OnStartup called");

    // Initialize PlayerBot hook system (BotNpcLocationService, GroupEventBus, etc.)
    TC_LOG_INFO("module.playerbot.script", "Initializing PlayerBot hook system...");
    Playerbot::PlayerBotHooks::Initialize();

    // INTEGRATION REQUIRED: Initialize companion systems (MountManager, BattlePetManager)
    // BLOCKED: These systems need API updates for TrinityCore 12.0 compatibility
    // API Changes Required:
    // - PassengerInfo boolean conversion -> use VehicleSeat::IsEmpty()
    // - SpellMgr::GetSpellInfo signature changed (now returns SpellInfo const*)
    // - Map::IsFlyingAllowed removed -> use CanFlyInZone() or check zone flags
    // - Map::IsArena removed -> use GetMapDifficulty()->IsArena()
    // Once fixed, uncomment:
    // TC_LOG_INFO("module.playerbot.script", "Initializing MountManager...");
    // Playerbot::MountManager::instance()->Initialize();
    // TC_LOG_INFO("module.playerbot.script", "Initializing BattlePetManager...");
    // Playerbot::BattlePetManager::instance()->Initialize();

    // Note: OnStartup is called before Playerbot module initialization,
    // so we defer enabling check to OnUpdate() when module is ready
    TC_LOG_INFO("module.playerbot.script", "PlayerbotWorldScript: All core systems initialized, deferring bot spawning to OnUpdate (module loads later)");
}

void PlayerbotWorldScript::OnShutdownInitiate(ShutdownExitCode code, ShutdownMask mask)
{
    if (!IsPlayerbotEnabled())
        return;

    TC_LOG_INFO("module.playerbot.script", "PlayerbotWorldScript: Shutdown initiated (code: {}, mask: {})",
                static_cast<uint32>(code), static_cast<uint32>(mask));

    try
    {
        // Ensure clean shutdown of all bot systems
    if (Playerbot::sBotSpawner)
        {
            TC_LOG_INFO("module.playerbot.script", "Despawning all active bots for shutdown");
            Playerbot::sBotSpawner->DespawnAllBots();
        }

        // Shutdown PlayerBot hook system
        TC_LOG_INFO("module.playerbot.script", "Shutting down PlayerBot hook system...");
        Playerbot::PlayerBotHooks::Shutdown();

        // Log final performance metrics
    if (_updateCount > 0)
        {
            float avgUpdateTime = static_cast<float>(_totalUpdateTime) / _updateCount;
            TC_LOG_INFO("module.playerbot.script",
                "PlayerbotWorldScript: Final metrics - {} updates, {:.2f}ms average",
                _updateCount, avgUpdateTime);
        }
    }
    catch (std::exception const& e)
    {
        TC_LOG_ERROR("module.playerbot.script",
            "PlayerbotWorldScript::OnShutdownInitiate: Exception during shutdown: {}", e.what());
    }
}

void PlayerbotWorldScript::UpdateBotSystems(uint32 diff)
{
    static uint32 lastDebugLog = 0;
    uint32 currentTime = GameTime::GetGameTimeMS();
    bool shouldLog = (currentTime - lastDebugLog > 5000); // Log every 5 seconds

    // Update BotSpawner for population management and character creation
    if (Playerbot::sBotSpawner)
    {
        try
        {
            Playerbot::sBotSpawner->Update(diff);
        }
        catch (std::exception const& e)
        {
            TC_LOG_ERROR("module.playerbot.script",
                "PlayerbotWorldScript::UpdateBotSystems: BotSpawner exception: {}", e.what());
        }
    }

    // ENTERPRISE FIX: Use BotWorldSessionMgr (correct system) instead of BotSessionMgr (legacy/unused)
    // Root Cause: Two competing session management systems exist:
    //   1. BotSessionMgr - Legacy system with empty _activeSessions (NOT used by BotSpawner)
    //   2. BotWorldSessionMgr - Active system with all bot sessions (used by BotSpawner)
    // BotSpawner calls sBotWorldSessionMgr->AddPlayerBot() which stores sessions in BotWorldSessionMgr
    // Therefore, we must call sBotWorldSessionMgr->UpdateSessions() to update those sessions
    if (Playerbot::sBotWorldSessionMgr)
    {
        if (shouldLog)
        {
            TC_LOG_INFO("module.playerbot.script", " UpdateBotSystems: Calling sBotWorldSessionMgr->UpdateSessions(), active bots: {}",
                        Playerbot::sBotWorldSessionMgr->GetBotCount());
        }
        try
        {
            Playerbot::sBotWorldSessionMgr->UpdateSessions(diff);

            // CRITICAL FIX: Process deferred packets on main thread (spell 49416 crash fix)
            // Bot worker threads classify packets - race-prone packets (spells, items, combat)
            // are deferred here for serialization with Map::Update() to prevent aura race conditions
    if (Playerbot::sBotWorldSessionMgr->IsEnabled())
            {
                uint32 processed = Playerbot::sBotWorldSessionMgr->ProcessAllDeferredPackets();
                if (processed > 0 && shouldLog)
                {
                    TC_LOG_DEBUG("playerbot.packets.deferred",
                        "PlayerbotWorldScript: Processed {} deferred packets on main thread", processed);
                }
            }
        }
        catch (std::exception const& e)
        {
            TC_LOG_ERROR("module.playerbot.script",
                "PlayerbotWorldScript::UpdateBotSystems: BotWorldSessionMgr exception: {}", e.what());
        }
    }
    else if (shouldLog)
    {
        TC_LOG_ERROR("module.playerbot.script", " UpdateBotSystems: sBotWorldSessionMgr is NULL!");
    }

    // ================================================================
    // CRITICAL FIX: Process BotLevelManager queue on main thread
    // ================================================================
    // BotLevelManager processes bot level-ups, gear, talents, and zone placement.
    // Worker threads prepare bot data asynchronously, then main thread applies it.
    // Without this call, bots get submitted but never leveled up!
    // ================================================================
    if (sBotLevelManager->IsReady())
    {
        try
        {
            uint32 processed = sBotLevelManager->ProcessBotCreationQueue(10);
            if (processed > 0 && shouldLog)
            {
                TC_LOG_INFO("module.playerbot.script",
                    "PlayerbotWorldScript: BotLevelManager processed {} bots", processed);
            }
        }
        catch (std::exception const& e)
        {
            TC_LOG_ERROR("module.playerbot.script",
                "PlayerbotWorldScript::UpdateBotSystems: BotLevelManager exception: {}", e.what());
        }

        // ================================================================
        // LEVEL DISTRIBUTION REBALANCING (every 5 minutes)
        // ================================================================
        // Periodically check if level distribution is balanced and trigger
        // redistribution if needed. This ensures bots across all level brackets
        // follow the configured distribution (5% Starting, 15% Chromie Time,
        // 20% Dragonflight, 60% TWW).
        // ================================================================
        if (currentTime - _lastRebalanceCheck >= REBALANCE_INTERVAL)
        {
            _lastRebalanceCheck = currentTime;

            try
            {
                if (!sBotLevelManager->IsDistributionBalanced())
                {
                    float deviation = sBotLevelManager->GetDistributionDeviation();
                    TC_LOG_INFO("module.playerbot.script",
                        "PlayerbotWorldScript: Level distribution deviation {:.1f}% - triggering rebalance",
                        deviation);
                    sBotLevelManager->RebalanceDistribution();
                }
                else if (shouldLog)
                {
                    TC_LOG_DEBUG("module.playerbot.script",
                        "PlayerbotWorldScript: Level distribution is balanced");
                }
            }
            catch (std::exception const& e)
            {
                TC_LOG_ERROR("module.playerbot.script",
                    "PlayerbotWorldScript::UpdateBotSystems: RebalanceDistribution exception: {}", e.what());
            }
        }
    }

    if (shouldLog)
        lastDebugLog = currentTime;

    // Update BotLifecycleMgr for bot lifecycle management
    // Note: This may be commented out in current implementation
    /*
    if (Playerbot::sBotLifecycleMgr)
    {
        Playerbot::sBotLifecycleMgr->Update(diff);
    }
    */
}

bool PlayerbotWorldScript::IsPlayerbotEnabled() const
{
    // Check if playerbot module is enabled
    if (!sPlayerbotConfig)
    {
        TC_LOG_DEBUG("module.playerbot.script", "IsPlayerbotEnabled: sPlayerbotConfig is null - module not initialized yet");
        return false;
    }

    bool enabled = sPlayerbotConfig->GetBool("Playerbot.Enable", false);
    static bool loggedOnce = false;
    if (!loggedOnce)
    {
        TC_LOG_INFO("module.playerbot.script", "IsPlayerbotEnabled: Config reads Playerbot.Enable = {} (config loaded: {}), forcing TRUE for testing",
                    enabled ? "true" : "false", sPlayerbotConfig ? "yes" : "no");
        loggedOnce = true;
    }

    // TEMPORARY: Force enable for testing the WorldScript integration
    return true;
}

void PlayerbotWorldScript::UpdateMetrics(uint32 updateTime)
{
    _totalUpdateTime += updateTime;
    _updateCount++;

    uint32 currentTime = GameTime::GetGameTimeMS();
    if (currentTime - _lastMetricUpdate >= METRIC_UPDATE_INTERVAL)
    {
        if (_updateCount > 0)
        {
            float avgUpdateTime = static_cast<float>(_totalUpdateTime) / _updateCount;

            TC_LOG_DEBUG("module.playerbot.script",
                "PlayerbotWorldScript: Performance metrics - {} updates, {:.2f}ms average",
                _updateCount, avgUpdateTime);

            // Performance warning if average is too high
    if (avgUpdateTime > 50.0f) // 50ms average threshold
            {
                TC_LOG_WARN("module.playerbot.script",
                    "PlayerbotWorldScript: High average update time: {:.2f}ms", avgUpdateTime);
            }
        }

        // Reset metrics for next interval
        _lastMetricUpdate = currentTime;
        _totalUpdateTime = 0;
        _updateCount = 0;
    }
}

// Forward declaration for playerbot command script
void AddSC_playerbot_commandscript();

// Forward declaration for Phase 4.1 event scripts
void AddSC_playerbot_event_scripts();

// Forward declaration for group event scripts
void AddSC_PlayerbotGroupScripts();

// Forward declaration for LFG bot integration scripts
namespace Playerbot { void AddSC_PlayerbotLFGScript(); }

// Forward declaration for Dragonriding system scripts
void AddSC_playerbot_dragonriding();
void AddSC_playerbot_dragonriding_glyphs();

// Forward declaration for BG/Arena bot integration scripts
namespace Playerbot { void AddSC_PlayerbotBGScript(); }
namespace Playerbot { void AddSC_PlayerbotArenaScript(); }

// Forward declaration for spell script fixes (null-safety patches for TrinityCore bugs)
void AddSC_playerbot_spell_fixes();

// Forward declaration for Instance Bot system scripts
namespace Playerbot { void RegisterInstanceBotScripts(); }

// Script registration function
void AddSC_playerbot_world()
{
    new PlayerbotWorldScript();

    // Register playerbot commands (module-only approach)
    #ifdef BUILD_PLAYERBOT
    AddSC_playerbot_commandscript();

    // Phase 4.1: Register event system scripts (non-invasive hooks)
    AddSC_playerbot_event_scripts();

    // Group Event System: Register group event handlers (ScriptMgr + polling)
    AddSC_PlayerbotGroupScripts();

    // LFG Bot Integration: Register LFG polling scripts (module-only, no core hooks)
    Playerbot::AddSC_PlayerbotLFGScript();

    // Dragonriding System: Register Soar, boost abilities, and glyph collection scripts
    AddSC_playerbot_dragonriding();
    AddSC_playerbot_dragonriding_glyphs();

    // BG/Arena Bot Integration: Register battleground and arena polling scripts
    Playerbot::AddSC_PlayerbotBGScript();
    Playerbot::AddSC_PlayerbotArenaScript();

    // Instance Bot System: Register pool management and LFG/BG integration scripts
    Playerbot::RegisterInstanceBotScripts();

    // Spell Script Fixes: Override buggy TrinityCore scripts with null-safe versions
    AddSC_playerbot_spell_fixes();

    // Dungeon Scripts: Load all dungeon-specific bot AI scripts
    Playerbot::DungeonScriptLoader::LoadDungeonScripts();
    #endif
}