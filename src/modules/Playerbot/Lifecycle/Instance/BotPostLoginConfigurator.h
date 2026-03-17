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
#include "ObjectGuid.h"
#include "BotTemplateRepository.h"
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <atomic>
#include <chrono>

class Player;

namespace Playerbot
{

// ============================================================================
// BotPendingConfiguration
// ============================================================================
// Stores configuration parameters for a bot that needs post-login setup.
// Created by JITBotFactory/BotCloneEngine, consumed by BotPostLoginConfigurator.
// ============================================================================

struct BotPendingConfiguration
{
    ObjectGuid botGuid;
    uint32 templateId = 0;
    uint32 targetLevel = 1;
    uint32 targetGearScore = 0;
    uint32 specId = 0;

    // Template pointer (cached for quick access)
    BotTemplate const* templatePtr = nullptr;

    // JIT Queue Configuration (set when bot is created for queue filling)
    uint32 dungeonIdToQueue = 0;     // If > 0, queue bot for this dungeon after configuration
    uint32 battlegroundIdToQueue = 0; // If > 0, queue bot for this BG after configuration
    uint32 arenaTypeToQueue = 0;      // If > 0, queue bot for this arena type after configuration

    // Human player GUID for BG/Arena queue tracking (required for invitation handling)
    ObjectGuid humanPlayerGuid;       // The human player that triggered this bot spawn

    // Instance Bot Flag - marks bot for idle timeout and restricted behavior
    bool markAsInstanceBot = false;  // If true, mark as instance bot after login

    // Timing
    std::chrono::steady_clock::time_point createdAt;

    // State tracking
    bool levelApplied = false;
    bool specApplied = false;
    bool talentsApplied = false;
    bool gearApplied = false;
    bool actionBarsApplied = false;
    bool spellsApplied = false;

    BotPendingConfiguration() : createdAt(std::chrono::steady_clock::now()) {}
};

// ============================================================================
// BotPostLoginConfigurator
// ============================================================================
// Enterprise-grade post-login configuration system for JIT-created bots.
//
// This class applies configuration (level, gear, talents, action bars) to bots
// AFTER they have fully logged in and entered the world. This is the correct
// approach because:
//
// 1. Player APIs (GiveLevel, EquipNewItem, LearnTalent) require a live Player
// 2. Direct DB manipulation before login doesn't properly initialize stats
// 3. Talents require specialization to be set first
// 4. Gear equipping needs proper inventory validation
//
// Usage:
//   1. JITBotFactory creates bot character (minimal DB record)
//   2. JITBotFactory registers pending configuration via RegisterPendingConfig()
//   3. Bot logs in via normal BotSession flow
//   4. BotSession::HandleBotPlayerLogin() calls ApplyPendingConfiguration()
//   5. BotPostLoginConfigurator applies level, spec, talents, gear using Player APIs
//
// Thread Safety:
//   - RegisterPendingConfig() is thread-safe (called from JIT worker)
//   - ApplyPendingConfiguration() is main-thread only
//   - HasPendingConfiguration() is thread-safe
//
// ============================================================================

class TC_GAME_API BotPostLoginConfigurator
{
public:
    static BotPostLoginConfigurator* Instance();

    // ========================================================================
    // LIFECYCLE
    // ========================================================================

    bool Initialize();
    void Shutdown();

    // ========================================================================
    // CONFIGURATION REGISTRATION (Thread-safe, called from JIT worker)
    // ========================================================================

    /// Register a bot for post-login configuration
    /// Called by JITBotFactory after creating the character record
    void RegisterPendingConfig(BotPendingConfiguration config);

    /// Check if a bot has pending configuration
    bool HasPendingConfiguration(ObjectGuid botGuid) const;

    /// Get pending configuration (returns nullptr if none)
    BotPendingConfiguration const* GetPendingConfiguration(ObjectGuid botGuid) const;

    /// Remove pending configuration (called after successful application)
    void RemovePendingConfiguration(ObjectGuid botGuid);

    /// Check if a bot was recently JIT-configured (prevents LevelManager re-leveling)
    /// This returns true for bots that have been configured but not yet processed by
    /// the BotWorldSessionMgr session update loop. This prevents the race condition where
    /// pending config is removed before the skip check runs.
    bool WasRecentlyConfigured(ObjectGuid botGuid) const;

    /// Clear a bot from the recently configured list (called after bot is fully processed)
    void ClearRecentlyConfigured(ObjectGuid botGuid);

    // ========================================================================
    // CONFIGURATION APPLICATION (Main thread only, called from BotSession)
    // ========================================================================

    /// Apply all pending configuration to a logged-in bot
    /// Returns true if configuration was applied successfully
    /// Called from BotSession::HandleBotPlayerLogin() after player is in world
    bool ApplyPendingConfiguration(Player* player);

    // ========================================================================
    // INDIVIDUAL CONFIGURATION STEPS (Can be called separately if needed)
    // ========================================================================

    /// Apply level to player (uses Player::GiveLevel)
    bool ApplyLevel(Player* player, uint32 targetLevel);

    /// Apply specialization (uses Player::SetPrimarySpecialization)
    bool ApplySpecialization(Player* player, uint32 specId);

    /// Apply talents from template (uses Player::LearnTalent)
    bool ApplyTalents(Player* player, BotTemplate const* tmpl);

    /// Apply gear from template (uses Player::EquipNewItem)
    bool ApplyGear(Player* player, BotTemplate const* tmpl, uint32 targetGearScore);

    /// Apply action bars from template
    bool ApplyActionBars(Player* player, BotTemplate const* tmpl);

    /// Learn class spells appropriate for level
    bool ApplyClassSpells(Player* player);

    // ========================================================================
    // STATISTICS
    // ========================================================================

    struct ConfiguratorStats
    {
        std::atomic<uint32> totalConfigured{0};
        std::atomic<uint32> successfulConfigs{0};
        std::atomic<uint32> failedConfigs{0};
        std::atomic<uint32> pendingConfigs{0};
        std::atomic<uint64> totalConfigTimeMs{0};

        float GetAverageConfigTimeMs() const
        {
            uint32 total = totalConfigured.load();
            return total > 0 ? static_cast<float>(totalConfigTimeMs.load()) / total : 0.0f;
        }

        float GetSuccessRate() const
        {
            uint32 total = totalConfigured.load();
            return total > 0 ? static_cast<float>(successfulConfigs.load()) / total * 100.0f : 0.0f;
        }
    };

    ConfiguratorStats const& GetStats() const { return _stats; }
    void ResetStats();

private:
    BotPostLoginConfigurator() = default;
    ~BotPostLoginConfigurator() = default;

    // ========================================================================
    // INTERNAL HELPERS
    // ========================================================================

    /// Select best gear set from template for target gear score
    GearSetTemplate const* SelectGearSet(BotTemplate const* tmpl, uint32 targetGearScore) const;

    /// Equip a single item
    bool EquipItem(Player* player, uint8 slot, uint32 itemId);

    /// Learn a single talent with error handling
    bool LearnTalent(Player* player, uint32 talentId);

    /// Learn essential class combat spells (trainer-learned abilities)
    void LearnEssentialClassSpells(Player* player);

    // ========================================================================
    // DATA MEMBERS
    // ========================================================================

    // Pending configurations (thread-safe access)
    mutable std::mutex _configMutex;
    std::unordered_map<ObjectGuid, BotPendingConfiguration> _pendingConfigs;

    // Recently configured bots - prevents BotLevelManager from re-leveling JIT bots
    // This tracks bots that have been configured but not yet processed by the
    // BotWorldSessionMgr session update loop. The race condition is:
    // 1. ApplyPendingConfiguration() applies level, removes pending config
    // 2. BotWorldSessionMgr.cpp checks HasPendingConfiguration() - returns FALSE
    // 3. Bot gets submitted to BotLevelManager which re-levels it
    // Solution: Track in _recentlyConfiguredBots until cleared by the session manager
    std::unordered_set<ObjectGuid> _recentlyConfiguredBots;

    // Statistics
    ConfiguratorStats _stats;

    // State
    std::atomic<bool> _initialized{false};
};

#define sBotPostLoginConfigurator Playerbot::BotPostLoginConfigurator::Instance()

} // namespace Playerbot
