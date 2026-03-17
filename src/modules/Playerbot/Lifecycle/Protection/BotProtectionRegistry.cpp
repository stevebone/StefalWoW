/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "BotProtectionRegistry.h"
#include "Config/PlayerbotConfig.h"
#include "Database/PlayerbotDatabase.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "GameTime.h"
#include "World.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "SocialMgr.h"
#include "GroupMgr.h"
#include "StringFormat.h"
#include <algorithm>

namespace Playerbot
{

// ============================================================================
// SINGLETON
// ============================================================================

BotProtectionRegistry* BotProtectionRegistry::Instance()
{
    static BotProtectionRegistry instance;
    return &instance;
}

// ============================================================================
// INITIALIZATION
// ============================================================================

bool BotProtectionRegistry::Initialize()
{
    if (_initialized.load())
        return true;

    TC_LOG_INFO("module.playerbot.protection", "Initializing BotProtectionRegistry");

    // Load configuration
    LoadConfig();

    // Load existing protection data from database
    LoadFromDatabase();

    // Refresh from game systems to ensure consistency
    RefreshFromGameSystems();

    _initialized.store(true);

    TC_LOG_INFO("module.playerbot.protection",
        "BotProtectionRegistry initialized with {} tracked bots ({} protected)",
        _protectionMap.size(), GetStatistics().protectedBots);

    return true;
}

void BotProtectionRegistry::Shutdown()
{
    if (!_initialized.load())
        return;

    TC_LOG_INFO("module.playerbot.protection", "Shutting down BotProtectionRegistry");

    // Save all dirty entries to database
    SaveToDatabase();

    // Clear providers
    ClearProviders();

    // Clear data
    _protectionMap.clear();
    _botLevels.clear();
    _friendReferences.clear();

    _initialized.store(false);

    TC_LOG_INFO("module.playerbot.protection", "BotProtectionRegistry shutdown complete");
}

void BotProtectionRegistry::Update(uint32 diff)
{
    if (!_initialized.load())
        return;

    // Time-based flag updates (every minute)
    _timeBasedUpdateAccumulator += diff;
    if (_timeBasedUpdateAccumulator >= TIME_BASED_UPDATE_INTERVAL_MS)
    {
        _timeBasedUpdateAccumulator = 0;
        UpdateTimeBasedFlags();
    }

    // Database sync
    if (_config.enableDbSync)
    {
        _dbSyncAccumulator += diff;
        if (_dbSyncAccumulator >= _config.dbSyncIntervalMs)
        {
            _dbSyncAccumulator = 0;
            SaveToDatabase();
        }
    }
}

void BotProtectionRegistry::LoadConfig()
{
    TC_LOG_DEBUG("module.playerbot.protection", "Loading protection configuration");

    // Protection enables - using PlayerbotConfig (reads from playerbots.conf)
    _config.enableGuildProtection = sPlayerbotConfig->GetBool("Playerbot.Lifecycle.Protection.Guild", true);
    _config.enableFriendProtection = sPlayerbotConfig->GetBool("Playerbot.Lifecycle.Protection.FriendList", true);
    _config.enableGroupProtection = sPlayerbotConfig->GetBool("Playerbot.Lifecycle.Protection.RecentGroup", true);
    _config.enableInteractionProtection = sPlayerbotConfig->GetBool("Playerbot.Lifecycle.Protection.Interaction", true);
    _config.enableMailProtection = sPlayerbotConfig->GetBool("Playerbot.Lifecycle.Protection.Mail", true);
    _config.enableAuctionProtection = sPlayerbotConfig->GetBool("Playerbot.Lifecycle.Protection.Auction", true);

    // Time windows
    _config.interactionWindowHours = sPlayerbotConfig->GetInt("Playerbot.Lifecycle.Protection.InteractionWindowHours", 24);

    // Database sync
    _config.dbSyncIntervalMs = sPlayerbotConfig->GetInt("Playerbot.Lifecycle.Protection.DbSyncIntervalMs", 60000);
    _config.enableDbSync = sPlayerbotConfig->GetBool("Playerbot.Lifecycle.Protection.DbSyncEnabled", true);

    TC_LOG_DEBUG("module.playerbot.protection",
        "Protection config: Guild={}, Friends={}, Groups={}, Interaction={}h, DbSync={}ms",
        _config.enableGuildProtection, _config.enableFriendProtection,
        _config.enableGroupProtection, _config.interactionWindowHours,
        _config.dbSyncIntervalMs);
}

// ============================================================================
// PROTECTION QUERIES
// ============================================================================

bool BotProtectionRegistry::IsProtected(ObjectGuid botGuid) const
{
    auto it = _protectionMap.find(botGuid);
    if (it == _protectionMap.end())
        return false;

    return it->second.IsProtected();
}

ProtectionStatus BotProtectionRegistry::GetStatus(ObjectGuid botGuid) const
{
    auto it = _protectionMap.find(botGuid);
    if (it == _protectionMap.end())
        return ProtectionStatus(botGuid);

    return it->second;
}

float BotProtectionRegistry::GetProtectionScore(ObjectGuid botGuid) const
{
    auto it = _protectionMap.find(botGuid);
    if (it == _protectionMap.end())
        return 0.0f;

    return it->second.protectionScore;
}

bool BotProtectionRegistry::HasProtectionReason(ObjectGuid botGuid, ProtectionReason reason) const
{
    auto it = _protectionMap.find(botGuid);
    if (it == _protectionMap.end())
        return false;

    return it->second.HasReason(reason);
}

uint32 BotProtectionRegistry::GetInteractionCount(ObjectGuid botGuid) const
{
    auto it = _protectionMap.find(botGuid);
    if (it == _protectionMap.end())
        return 0;

    return it->second.interactionCount;
}

// ============================================================================
// BULK QUERIES
// ============================================================================

std::vector<ObjectGuid> BotProtectionRegistry::GetUnprotectedBotsInBracket(LevelBracket const* bracket) const
{
    std::vector<ObjectGuid> result;
    if (!bracket)
        return result;

    uint32 minLevel = bracket->minLevel;
    uint32 maxLevel = bracket->maxLevel;

    _botLevels.for_each([&](auto const& pair) {
        ObjectGuid guid = pair.first;
        uint32 level = pair.second;

        if (level >= minLevel && level <= maxLevel)
        {
            auto protIt = _protectionMap.find(guid);
            if (protIt == _protectionMap.end() || !protIt->second.IsProtected())
            {
                result.push_back(guid);
            }
        }
    });

    return result;
}

std::vector<ObjectGuid> BotProtectionRegistry::GetProtectedBotsInBracket(LevelBracket const* bracket) const
{
    std::vector<ObjectGuid> result;
    if (!bracket)
        return result;

    uint32 minLevel = bracket->minLevel;
    uint32 maxLevel = bracket->maxLevel;

    _botLevels.for_each([&](auto const& pair) {
        ObjectGuid guid = pair.first;
        uint32 level = pair.second;

        if (level >= minLevel && level <= maxLevel)
        {
            auto protIt = _protectionMap.find(guid);
            if (protIt != _protectionMap.end() && protIt->second.IsProtected())
            {
                result.push_back(guid);
            }
        }
    });

    return result;
}

uint32 BotProtectionRegistry::GetProtectedCountInBracket(LevelBracket const* bracket) const
{
    return static_cast<uint32>(GetProtectedBotsInBracket(bracket).size());
}

uint32 BotProtectionRegistry::GetUnprotectedCountInBracket(LevelBracket const* bracket) const
{
    return static_cast<uint32>(GetUnprotectedBotsInBracket(bracket).size());
}

std::vector<ObjectGuid> BotProtectionRegistry::GetRetirementCandidates(LevelBracket const* bracket, uint32 maxCount) const
{
    // Get all unprotected bots in bracket
    auto candidates = GetUnprotectedBotsInBracket(bracket);

    // Build list with scores for sorting
    std::vector<std::pair<ObjectGuid, float>> scoredCandidates;
    scoredCandidates.reserve(candidates.size());

    for (ObjectGuid guid : candidates)
    {
        float score = GetProtectionScore(guid);
        scoredCandidates.emplace_back(guid, score);
    }

    // Sort by score ascending (lowest score = best retirement candidate)
    std::sort(scoredCandidates.begin(), scoredCandidates.end(),
        [](auto const& a, auto const& b) {
            return a.second < b.second;
        });

    // Take top N candidates
    std::vector<ObjectGuid> result;
    uint32 count = std::min(maxCount, static_cast<uint32>(scoredCandidates.size()));
    result.reserve(count);

    for (uint32 i = 0; i < count; ++i)
    {
        result.push_back(scoredCandidates[i].first);
    }

    return result;
}

// ============================================================================
// PROTECTION EVENTS
// ============================================================================

void BotProtectionRegistry::OnBotJoinedGuild(ObjectGuid botGuid, ObjectGuid guildGuid)
{
    if (!_config.enableGuildProtection)
        return;

    TC_LOG_DEBUG("module.playerbot.protection",
        "Bot {} joined guild {}", botGuid.ToString(), guildGuid.ToString());

    auto& status = GetOrCreateStatus(botGuid);
    status.SetGuild(guildGuid);
    status.RecalculateScore();
    MarkDirty(botGuid);

    _statsDirty.store(true);
}

void BotProtectionRegistry::OnBotLeftGuild(ObjectGuid botGuid)
{
    if (!_config.enableGuildProtection)
        return;

    TC_LOG_DEBUG("module.playerbot.protection", "Bot {} left guild", botGuid.ToString());

    auto it = _protectionMap.find(botGuid);
    if (it != _protectionMap.end())
    {
        it->second.SetGuild(ObjectGuid::Empty);
        it->second.RecalculateScore();
        MarkDirty(botGuid);
        _statsDirty.store(true);
    }
}

void BotProtectionRegistry::OnPlayerAddedFriend(ObjectGuid playerGuid, ObjectGuid botGuid)
{
    if (!_config.enableFriendProtection)
        return;

    TC_LOG_DEBUG("module.playerbot.protection",
        "Player {} added bot {} as friend", playerGuid.ToString(), botGuid.ToString());

    // Update friend references map
    auto& friendSet = _friendReferences[playerGuid];
    friendSet.insert(botGuid);

    // Update bot's protection status
    auto& status = GetOrCreateStatus(botGuid);
    status.AddFriendReference(playerGuid);
    status.RecalculateScore();
    MarkDirty(botGuid);

    _statsDirty.store(true);
}

void BotProtectionRegistry::OnPlayerRemovedFriend(ObjectGuid playerGuid, ObjectGuid botGuid)
{
    if (!_config.enableFriendProtection)
        return;

    TC_LOG_DEBUG("module.playerbot.protection",
        "Player {} removed bot {} from friends", playerGuid.ToString(), botGuid.ToString());

    // Update friend references map
    auto it = _friendReferences.find(playerGuid);
    if (it != _friendReferences.end())
    {
        it->second.erase(botGuid);
        if (it->second.empty())
        {
            _friendReferences.erase(it);
        }
    }

    // Update bot's protection status
    auto protIt = _protectionMap.find(botGuid);
    if (protIt != _protectionMap.end())
    {
        protIt->second.RemoveFriendReference(playerGuid);
        protIt->second.RecalculateScore();
        MarkDirty(botGuid);
        _statsDirty.store(true);
    }
}

void BotProtectionRegistry::OnPlayerInteraction(ObjectGuid playerGuid, ObjectGuid botGuid)
{
    if (!_config.enableInteractionProtection)
        return;

    TC_LOG_TRACE("module.playerbot.protection",
        "Player {} interacted with bot {}", playerGuid.ToString(), botGuid.ToString());

    auto& status = GetOrCreateStatus(botGuid);
    status.RecordInteraction();
    MarkDirty(botGuid);
}

void BotProtectionRegistry::OnBotGroupedWithPlayer(ObjectGuid botGuid, ObjectGuid playerGuid)
{
    if (!_config.enableGroupProtection)
        return;

    TC_LOG_DEBUG("module.playerbot.protection",
        "Bot {} grouped with player {}", botGuid.ToString(), playerGuid.ToString());

    auto& status = GetOrCreateStatus(botGuid);
    status.RecordGroupJoin(playerGuid);  // Uses playerGuid as group identifier
    MarkDirty(botGuid);

    _statsDirty.store(true);
}

void BotProtectionRegistry::OnBotLeftGroup(ObjectGuid botGuid)
{
    if (!_config.enableGroupProtection)
        return;

    TC_LOG_DEBUG("module.playerbot.protection", "Bot {} left group", botGuid.ToString());

    auto it = _protectionMap.find(botGuid);
    if (it != _protectionMap.end())
    {
        it->second.RecordGroupLeave();
        MarkDirty(botGuid);
        _statsDirty.store(true);
    }
}

void BotProtectionRegistry::OnBotMailReceived(ObjectGuid botGuid)
{
    if (!_config.enableMailProtection)
        return;

    TC_LOG_DEBUG("module.playerbot.protection", "Bot {} received mail", botGuid.ToString());

    auto& status = GetOrCreateStatus(botGuid);
    status.SetMailStatus(true);
    status.RecalculateScore();
    MarkDirty(botGuid);

    _statsDirty.store(true);
}

void BotProtectionRegistry::OnBotMailCleared(ObjectGuid botGuid)
{
    if (!_config.enableMailProtection)
        return;

    TC_LOG_DEBUG("module.playerbot.protection", "Bot {} mail cleared", botGuid.ToString());

    auto it = _protectionMap.find(botGuid);
    if (it != _protectionMap.end())
    {
        it->second.SetMailStatus(false);
        it->second.RecalculateScore();
        MarkDirty(botGuid);
        _statsDirty.store(true);
    }
}

void BotProtectionRegistry::OnBotAuctionCreated(ObjectGuid botGuid)
{
    if (!_config.enableAuctionProtection)
        return;

    TC_LOG_DEBUG("module.playerbot.protection", "Bot {} created auction", botGuid.ToString());

    auto& status = GetOrCreateStatus(botGuid);
    status.SetAuctionStatus(true);
    status.RecalculateScore();
    MarkDirty(botGuid);

    _statsDirty.store(true);
}

void BotProtectionRegistry::OnBotAuctionCleared(ObjectGuid botGuid)
{
    if (!_config.enableAuctionProtection)
        return;

    TC_LOG_DEBUG("module.playerbot.protection", "Bot {} auctions cleared", botGuid.ToString());

    auto it = _protectionMap.find(botGuid);
    if (it != _protectionMap.end())
    {
        it->second.SetAuctionStatus(false);
        it->second.RecalculateScore();
        MarkDirty(botGuid);
        _statsDirty.store(true);
    }
}

void BotProtectionRegistry::OnBotCreated(ObjectGuid botGuid, uint32 level)
{
    TC_LOG_DEBUG("module.playerbot.protection",
        "Bot {} created at level {}", botGuid.ToString(), level);

    // Create initial protection status
    auto& status = GetOrCreateStatus(botGuid);
    status.protectionFlags = ProtectionReason::None;
    status.RecalculateScore();

    // Track level
    _botLevels[botGuid] = level;

    MarkDirty(botGuid);
    _statsDirty.store(true);
}

void BotProtectionRegistry::OnBotDeleted(ObjectGuid botGuid)
{
    TC_LOG_DEBUG("module.playerbot.protection", "Bot {} deleted", botGuid.ToString());

    // Remove from protection map
    _protectionMap.erase(botGuid);

    // Remove from level tracking
    _botLevels.erase(botGuid);

    // Remove from dirty set
    {
        std::lock_guard<std::mutex> lock(_dirtyMutex);
        _dirtyBots.erase(botGuid);
    }

    // Remove from friend references - collect keys first, then modify
    std::vector<ObjectGuid> keysToModify;
    _friendReferences.for_each([&](auto const& pair) {
        if (pair.second.count(botGuid) > 0)
            keysToModify.push_back(pair.first);
    });
    for (ObjectGuid const& key : keysToModify)
    {
        _friendReferences.modify_if(key, [&botGuid](auto& pair) {
            pair.second.erase(botGuid);
        });
    }

    _statsDirty.store(true);
}

void BotProtectionRegistry::OnBotLevelUp(ObjectGuid botGuid, uint32 /*oldLevel*/, uint32 newLevel)
{
    TC_LOG_TRACE("module.playerbot.protection",
        "Bot {} leveled up to {}", botGuid.ToString(), newLevel);

    _botLevels[botGuid] = newLevel;
    _statsDirty.store(true);
}

// ============================================================================
// ADMINISTRATIVE
// ============================================================================

void BotProtectionRegistry::SetManualProtection(ObjectGuid botGuid, bool protect)
{
    TC_LOG_INFO("module.playerbot.protection",
        "{} manual protection for bot {}",
        protect ? "Enabling" : "Disabling", botGuid.ToString());

    auto& status = GetOrCreateStatus(botGuid);
    status.SetManualProtection(protect);
    status.RecalculateScore();
    MarkDirty(botGuid);

    _statsDirty.store(true);
}

void BotProtectionRegistry::RecalculateAllProtection()
{
    TC_LOG_INFO("module.playerbot.protection", "Recalculating all bot protection statuses");

    // Collect all guids first, then modify each
    std::vector<ObjectGuid> allGuids;
    _protectionMap.for_each([&](auto const& pair) {
        allGuids.push_back(pair.first);
    });

    uint32 count = 0;
    uint32 interactionWindowHours = _config.interactionWindowHours;
    for (ObjectGuid const& guid : allGuids)
    {
        _protectionMap.modify_if(guid, [interactionWindowHours](auto& pair) {
            pair.second.UpdateTimeBasedFlags(interactionWindowHours);
            pair.second.RecalculateScore();
        });
        ++count;
    }

    // Mark all as dirty for database sync
    {
        std::lock_guard<std::mutex> lock(_dirtyMutex);
        _protectionMap.for_each([&](auto const& pair) {
            _dirtyBots.insert(pair.first);
        });
    }

    _statsDirty.store(true);

    TC_LOG_INFO("module.playerbot.protection", "Recalculated protection for {} bots", count);
}

void BotProtectionRegistry::RefreshFromGameSystems()
{
    TC_LOG_INFO("module.playerbot.protection", "Refreshing protection from game systems");

    // Query all registered providers
    std::shared_lock<std::shared_mutex> lock(_providerMutex);
    for (auto const& provider : _providers)
    {
        if (!provider)
            continue;

        auto protectedBots = provider->GetAllProtectedBots();
        for (ObjectGuid botGuid : protectedBots)
        {
            auto info = provider->GetProtectionInfo(botGuid);
            if (info.GrantsProtection())
            {
                auto& status = GetOrCreateStatus(botGuid);
                status.AddReason(info.reason);
                status.RecalculateScore();
                MarkDirty(botGuid);
            }
        }
    }

    _statsDirty.store(true);
}

ProtectionStatistics BotProtectionRegistry::GetStatistics() const
{
    if (!_statsDirty.load())
        return _cachedStats;

    // Recalculate statistics
    ProtectionStatistics stats;
    stats.lastUpdate = std::chrono::system_clock::now();

    float totalScore = 0.0f;
    stats.minProtectionScore = std::numeric_limits<float>::max();
    stats.maxProtectionScore = std::numeric_limits<float>::lowest();

    _protectionMap.for_each([&](auto const& pair) {
        ++stats.totalTrackedBots;

        ProtectionStatus const& status = pair.second;

        if (status.IsProtected())
            ++stats.protectedBots;
        else
            ++stats.unprotectedBots;

        // By reason
        if (status.HasReason(ProtectionReason::InGuild))
            ++stats.botsInGuild;
        if (status.HasReason(ProtectionReason::OnFriendList))
            ++stats.botsOnFriendList;
        if (status.HasReason(ProtectionReason::InPlayerGroup))
            ++stats.botsInPlayerGroup;
        if (status.HasReason(ProtectionReason::RecentInteract))
            ++stats.botsWithRecentInteraction;
        if (status.HasReason(ProtectionReason::HasActiveMail))
            ++stats.botsWithMail;
        if (status.HasReason(ProtectionReason::HasActiveAuction))
            ++stats.botsWithAuctions;
        if (status.HasReason(ProtectionReason::ManualProtect))
            ++stats.botsManuallyProtected;

        // Score distribution
        totalScore += status.protectionScore;
        if (status.protectionScore < stats.minProtectionScore)
            stats.minProtectionScore = status.protectionScore;
        if (status.protectionScore > stats.maxProtectionScore)
            stats.maxProtectionScore = status.protectionScore;
    });

    // Average score
    if (stats.totalTrackedBots > 0)
        stats.avgProtectionScore = totalScore / stats.totalTrackedBots;

    // Handle edge case of no bots
    if (stats.minProtectionScore == std::numeric_limits<float>::max())
        stats.minProtectionScore = 0.0f;
    if (stats.maxProtectionScore == std::numeric_limits<float>::lowest())
        stats.maxProtectionScore = 0.0f;

    // By bracket (using level tracking)
    _botLevels.for_each([&](auto const& pair) {
        ObjectGuid guid = pair.first;
        uint32 level = pair.second;

        uint32 bracketIdx = 0;
        if (level <= 10) bracketIdx = 0;
        else if (level <= 60) bracketIdx = 1;
        else if (level <= 70) bracketIdx = 2;
        else bracketIdx = 3;

        auto protIt = _protectionMap.find(guid);
        bool isProtected = (protIt != _protectionMap.end() && protIt->second.IsProtected());

        if (isProtected)
            ++stats.protectedPerBracket[bracketIdx];
        else
            ++stats.unprotectedPerBracket[bracketIdx];
    });

    _cachedStats = stats;
    _statsDirty.store(false);

    return stats;
}

void BotProtectionRegistry::PrintReport() const
{
    auto stats = GetStatistics();

    TC_LOG_INFO("module.playerbot.protection", "=== Bot Protection Report ===");
    TC_LOG_INFO("module.playerbot.protection", "Total Tracked: {} (Protected: {}, Unprotected: {})",
        stats.totalTrackedBots, stats.protectedBots, stats.unprotectedBots);

    TC_LOG_INFO("module.playerbot.protection", "--- Protection Reasons ---");
    TC_LOG_INFO("module.playerbot.protection", "  In Guild: {}", stats.botsInGuild);
    TC_LOG_INFO("module.playerbot.protection", "  On Friend List: {}", stats.botsOnFriendList);
    TC_LOG_INFO("module.playerbot.protection", "  In Player Group: {}", stats.botsInPlayerGroup);
    TC_LOG_INFO("module.playerbot.protection", "  Recent Interaction: {}", stats.botsWithRecentInteraction);
    TC_LOG_INFO("module.playerbot.protection", "  Has Mail: {}", stats.botsWithMail);
    TC_LOG_INFO("module.playerbot.protection", "  Has Auctions: {}", stats.botsWithAuctions);
    TC_LOG_INFO("module.playerbot.protection", "  Manual Protection: {}", stats.botsManuallyProtected);

    TC_LOG_INFO("module.playerbot.protection", "--- By Bracket ---");
    TC_LOG_INFO("module.playerbot.protection", "  Starting (1-10):     P={} U={}",
        stats.protectedPerBracket[0], stats.unprotectedPerBracket[0]);
    TC_LOG_INFO("module.playerbot.protection", "  ChromieTime (10-60): P={} U={}",
        stats.protectedPerBracket[1], stats.unprotectedPerBracket[1]);
    TC_LOG_INFO("module.playerbot.protection", "  Dragonflight (60-70):P={} U={}",
        stats.protectedPerBracket[2], stats.unprotectedPerBracket[2]);
    TC_LOG_INFO("module.playerbot.protection", "  TheWarWithin (70-80):P={} U={}",
        stats.protectedPerBracket[3], stats.unprotectedPerBracket[3]);

    TC_LOG_INFO("module.playerbot.protection", "--- Protection Scores ---");
    TC_LOG_INFO("module.playerbot.protection", "  Min: {:.1f}, Max: {:.1f}, Avg: {:.1f}",
        stats.minProtectionScore, stats.maxProtectionScore, stats.avgProtectionScore);
}

// ============================================================================
// PROVIDER REGISTRATION
// ============================================================================

void BotProtectionRegistry::RegisterProvider(std::shared_ptr<IProtectionProvider> provider)
{
    if (!provider)
        return;

    std::unique_lock<std::shared_mutex> lock(_providerMutex);
    _providers.push_back(std::move(provider));

    TC_LOG_DEBUG("module.playerbot.protection", "Registered protection provider: {}",
        _providers.back()->GetProviderName());
}

void BotProtectionRegistry::ClearProviders()
{
    std::unique_lock<std::shared_mutex> lock(_providerMutex);
    for (auto& provider : _providers)
    {
        if (provider)
            provider->ClearChangeCallbacks();
    }
    _providers.clear();
}

void BotProtectionRegistry::SetConfig(ProtectionConfig const& config)
{
    _config = config;
    RecalculateAllProtection();
}

// ============================================================================
// INTERNAL METHODS
// ============================================================================

ProtectionStatus& BotProtectionRegistry::GetOrCreateStatus(ObjectGuid botGuid)
{
    auto it = _protectionMap.find(botGuid);
    if (it != _protectionMap.end())
        return it->second;

    // Create new status
    ProtectionStatus newStatus(botGuid);
    auto result = _protectionMap.emplace(botGuid, std::move(newStatus));
    return result.first->second;
}

void BotProtectionRegistry::UpdateProtectionStatus(ObjectGuid botGuid)
{
    auto it = _protectionMap.find(botGuid);
    if (it == _protectionMap.end())
        return;

    it->second.UpdateTimeBasedFlags(_config.interactionWindowHours);
    it->second.RecalculateScore();
    MarkDirty(botGuid);
}

void BotProtectionRegistry::RecalculateScore(ObjectGuid botGuid)
{
    auto it = _protectionMap.find(botGuid);
    if (it != _protectionMap.end())
    {
        it->second.RecalculateScore();
        MarkDirty(botGuid);
    }
}

void BotProtectionRegistry::UpdateTimeBasedFlags()
{
    TC_LOG_TRACE("module.playerbot.protection", "Updating time-based protection flags");

    // Collect all guids first
    std::vector<ObjectGuid> allGuids;
    _protectionMap.for_each([&](auto const& pair) {
        allGuids.push_back(pair.first);
    });

    uint32 expiredCount = 0;
    uint32 interactionWindowHours = _config.interactionWindowHours;
    for (ObjectGuid const& guid : allGuids)
    {
        bool expired = false;
        _protectionMap.modify_if(guid, [interactionWindowHours, &expired](auto& pair) {
            bool hadRecentInteract = pair.second.HasReason(ProtectionReason::RecentInteract);
            pair.second.UpdateTimeBasedFlags(interactionWindowHours);
            expired = hadRecentInteract && !pair.second.HasReason(ProtectionReason::RecentInteract);
        });
        if (expired)
        {
            ++expiredCount;
            MarkDirty(guid);
        }
    }

    if (expiredCount > 0)
    {
        TC_LOG_DEBUG("module.playerbot.protection",
            "Expired RecentInteract protection for {} bots", expiredCount);
        _statsDirty.store(true);
    }
}

void BotProtectionRegistry::LoadFromDatabase()
{
    TC_LOG_INFO("module.playerbot.protection", "Loading protection data from database");

    // Load protection statuses from playerbot database
    QueryResult result = sPlayerbotDatabase->Query(
        "SELECT bot_guid, protection_flags, guild_guid, friend_count, interaction_count, "
        "UNIX_TIMESTAMP(last_interaction), UNIX_TIMESTAMP(last_group_time), protection_score "
        "FROM playerbot_protection_status");

    if (result)
    {
        uint32 count = 0;
        do
        {
            Field* fields = result->Fetch();

            ObjectGuid botGuid = ObjectGuid::Create<HighGuid::Player>(fields[0].GetUInt64());
            ProtectionStatus status(botGuid);

            status.protectionFlags = static_cast<ProtectionReason>(fields[1].GetUInt8());
            uint64 guildGuidLow = fields[2].GetUInt64();
            if (guildGuidLow > 0)
                status.guildGuid = ObjectGuid::Create<HighGuid::Guild>(guildGuidLow);
            status.friendCount = fields[3].GetUInt32();
            status.interactionCount = fields[4].GetUInt32();

            time_t lastInteract = fields[5].GetInt64();
            if (lastInteract > 0)
                status.lastInteraction = std::chrono::system_clock::from_time_t(lastInteract);

            time_t lastGroup = fields[6].GetInt64();
            if (lastGroup > 0)
                status.lastGroupTime = std::chrono::system_clock::from_time_t(lastGroup);

            status.protectionScore = fields[7].GetFloat();
            status.isDirty = false;

            _protectionMap[botGuid] = std::move(status);
            ++count;
        }
        while (result->NextRow());

        TC_LOG_INFO("module.playerbot.protection", "Loaded {} bot protection records", count);
    }

    // Load friend references from playerbot database
    result = sPlayerbotDatabase->Query(
        "SELECT bot_guid, player_guid FROM playerbot_friend_references");

    if (result)
    {
        uint32 count = 0;
        do
        {
            Field* fields = result->Fetch();

            ObjectGuid botGuid = ObjectGuid::Create<HighGuid::Player>(fields[0].GetUInt64());
            ObjectGuid playerGuid = ObjectGuid::Create<HighGuid::Player>(fields[1].GetUInt64());

            auto& friendSet = _friendReferences[playerGuid];
            friendSet.insert(botGuid);

            // Also update the bot's friend set
            auto it = _protectionMap.find(botGuid);
            if (it != _protectionMap.end())
            {
                it->second.friendingPlayers.insert(playerGuid);
            }

            ++count;
        }
        while (result->NextRow());

        TC_LOG_INFO("module.playerbot.protection", "Loaded {} friend references", count);
    }
}

void BotProtectionRegistry::SaveToDatabase()
{
    std::set<ObjectGuid> toSave;
    {
        std::lock_guard<std::mutex> lock(_dirtyMutex);
        toSave = std::move(_dirtyBots);
        _dirtyBots.clear();
    }

    if (toSave.empty())
        return;

    TC_LOG_DEBUG("module.playerbot.protection", "Saving {} dirty protection records to database", toSave.size());

    // Save each dirty record to playerbot database
    for (ObjectGuid botGuid : toSave)
    {
        auto it = _protectionMap.find(botGuid);
        if (it != _protectionMap.end())
        {
            SaveBotToDatabase(botGuid, it->second);
        }
    }
}

void BotProtectionRegistry::SaveBotToDatabase(ObjectGuid botGuid, ProtectionStatus const& status)
{
    // Convert timestamps to MySQL format
    auto lastInteractTime = std::chrono::system_clock::to_time_t(status.lastInteraction);
    auto lastGroupTime = std::chrono::system_clock::to_time_t(status.lastGroupTime);

    // Use REPLACE INTO for upsert behavior - save to playerbot database
    std::string query = Trinity::StringFormat(
        "REPLACE INTO playerbot_protection_status "
        "(bot_guid, protection_flags, guild_guid, friend_count, interaction_count, "
        "last_interaction, last_group_time, protection_score) "
        "VALUES ({}, {}, {}, {}, {}, FROM_UNIXTIME({}), FROM_UNIXTIME({}), {})",
        botGuid.GetCounter(),
        static_cast<uint8>(status.protectionFlags),
        status.guildGuid.IsEmpty() ? 0 : status.guildGuid.GetCounter(),
        status.friendCount,
        status.interactionCount,
        lastInteractTime > 0 ? lastInteractTime : 0,
        lastGroupTime > 0 ? lastGroupTime : 0,
        status.protectionScore
    );
    sPlayerbotDatabase->Execute(query);
}

void BotProtectionRegistry::MarkDirty(ObjectGuid botGuid)
{
    std::lock_guard<std::mutex> lock(_dirtyMutex);
    _dirtyBots.insert(botGuid);
}

LevelBracket const* BotProtectionRegistry::GetBracketForLevel(uint32 level) const
{
    // Use BotLevelDistribution to get bracket
    return sBotLevelDistribution->GetBracketForLevel(level, TEAM_NEUTRAL);
}

uint32 BotProtectionRegistry::GetBracketIndex(ExpansionTier tier) const
{
    return static_cast<uint32>(tier);
}

} // namespace Playerbot
