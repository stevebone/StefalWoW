/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "MountCollectionManager.h"
#include "AI/BotAI.h"
#include "Player.h"
#include "Companion/MountManager.h"
#include "DB2Stores.h"
#include "SpellMgr.h"
#include "SpellInfo.h"
#include "Log.h"
#include "World.h"
#include "GameTime.h"
#include "InstanceLockMgr.h"
#include "ReputationMgr.h"
#include "AchievementMgr.h"
#include <algorithm>

namespace Playerbot
{

// Static member definitions
MountCollectionManager::CollectionStatistics MountCollectionManager::_globalStatistics;
std::unordered_map<uint32, CollectibleMount> MountCollectionManager::_mountDatabase;
bool MountCollectionManager::_databaseLoaded = false;

MountCollectionManager::MountCollectionManager(Player* bot, BotAI* ai)
    : BehaviorManager(bot, ai, 10000, "MountCollectionManager")  // 10 second update
{
    // Enable all sources by default
    _enabledSources.insert(MountSource::VENDOR);
    _enabledSources.insert(MountSource::REPUTATION);
    _enabledSources.insert(MountSource::RAID_DROP);
    _enabledSources.insert(MountSource::DUNGEON_DROP);
    _enabledSources.insert(MountSource::WORLD_DROP);
    _enabledSources.insert(MountSource::ACHIEVEMENT);
    _enabledSources.insert(MountSource::QUEST);
    _enabledSources.insert(MountSource::PROFESSION);
}

bool MountCollectionManager::OnInitialize()
{
    if (!GetBot() || !GetBot()->IsInWorld())
        return false;

    // Load static mount database once
    if (!_databaseLoaded)
    {
        LoadMountDatabase();
        _databaseLoaded = true;
    }

    // Analyze mounts for this bot
    AnalyzeMounts();
    _lastAnalysis = std::chrono::steady_clock::now();

    TC_LOG_DEBUG("module.playerbot.mounts",
        "MountCollectionManager: Initialized for {} with {} owned mounts, {} collectible",
        GetBot()->GetName(), _ownedMounts.load(), _collectibleMounts.size());

    return true;
}

void MountCollectionManager::OnShutdown()
{
    if (_currentSession.isActive)
    {
        StopFarming("Shutdown");
    }

    _collectibleMounts.clear();
    _ownedMountSpells.clear();
}

void MountCollectionManager::OnUpdate(uint32 elapsed)
{
    if (!GetBot() || !GetBot()->IsInWorld())
        return;

    // Re-analyze mounts periodically
    auto now = std::chrono::steady_clock::now();
    auto timeSinceAnalysis = std::chrono::duration_cast<std::chrono::milliseconds>(now - _lastAnalysis).count();
    if (timeSinceAnalysis >= ANALYSIS_INTERVAL_MS)
    {
        AnalyzeMounts();
        _lastAnalysis = now;
    }

    // Update farming session if active
    if (_currentSession.isActive)
    {
        UpdateFarmingSession(elapsed);
    }
}

void MountCollectionManager::AnalyzeMounts()
{
    if (!GetBot())
        return;

    // Clear and rebuild collectible list
    _collectibleMounts.clear();
    _ownedMountSpells.clear();

    // Get owned mounts from MountManager
    MountManager* mountMgr = GetMountManager();
    if (mountMgr)
    {
        auto playerMounts = mountMgr->GetPlayerMounts();
        for (auto const& mount : playerMounts)
        {
            _ownedMountSpells.insert(mount.spellId);
        }
        _ownedMounts.store(static_cast<uint32>(_ownedMountSpells.size()), std::memory_order_release);
    }

    // Build collectible mount list from database
    for (auto const& [spellId, mountData] : _mountDatabase)
    {
        CollectibleMount mount = mountData;
        mount.isOwned = _ownedMountSpells.count(spellId) > 0;

        // Check if mount is farmable by this bot
        mount.isFarmable = !mount.isOwned && MeetsMountRequirements(mount);

        // Only add unowned mounts that are potentially obtainable
        if (!mount.isOwned && IsSourceEnabled(mount.source))
        {
            _collectibleMounts.push_back(mount);
        }
    }

    // Sort by priority
    std::sort(_collectibleMounts.begin(), _collectibleMounts.end(),
        [](CollectibleMount const& a, CollectibleMount const& b) {
            return a.GetPriorityScore() > b.GetPriorityScore();
        });

    TC_LOG_DEBUG("module.playerbot.mounts",
        "MountCollectionManager: {} analyzed mounts, {} owned, {} collectible",
        GetBot()->GetName(), _ownedMounts.load(), _collectibleMounts.size());
}

void MountCollectionManager::UpdateFarmingSession(uint32 /*elapsed*/)
{
    if (!_currentSession.isActive)
        return;

    // Check if we obtained the target mount
    CheckMountObtained();

    // Check max duration
    if (_currentSession.GetElapsedMs() > MAX_FARM_DURATION_MS)
    {
        StopFarming("Max duration reached");
        return;
    }

    // Execute farming step based on source
    switch (_currentSession.source)
    {
        case MountSource::RAID_DROP:
            ExecuteRaidFarmStep();
            break;
        case MountSource::REPUTATION:
            ExecuteRepFarmStep();
            break;
        case MountSource::DUNGEON_DROP:
            ExecuteDungeonFarmStep();
            break;
        case MountSource::VENDOR:
            ExecuteVendorFarmStep();
            break;
        default:
            break;
    }
}

void MountCollectionManager::ExecuteRaidFarmStep()
{
    // Navigate to raid if not in instance
    if (!_currentSession.isInInstance)
    {
        if (!_currentSession.isNavigating)
        {
            if (!NavigateToFarmLocation())
            {
                StopFarming("Failed to navigate to raid");
            }
        }
        return;
    }

    // In raid - combat/progression handled by other systems
    // This manager tracks mount-specific state
}

void MountCollectionManager::ExecuteRepFarmStep()
{
    // Reputation grinding handled by ReputationGrindManager
    // We just track progress toward mount unlock
    auto it = std::find_if(_collectibleMounts.begin(), _collectibleMounts.end(),
        [this](CollectibleMount const& m) { return m.spellId == _currentSession.targetMountSpellId; });

    if (it != _collectibleMounts.end() && it->requiredReputation > 0)
    {
        float progress = GetRepMountProgress(_currentSession.targetMountSpellId);
        if (progress >= 1.0f)
        {
            // Reputation requirement met - try to purchase
            ExecuteVendorFarmStep();
        }
    }
}

void MountCollectionManager::ExecuteDungeonFarmStep()
{
    // Similar to raid farming but for dungeons
    if (!_currentSession.isInInstance)
    {
        if (!_currentSession.isNavigating)
        {
            if (!NavigateToFarmLocation())
            {
                StopFarming("Failed to navigate to dungeon");
            }
        }
    }
}

void MountCollectionManager::ExecuteVendorFarmStep()
{
    // Find mount data
    auto it = std::find_if(_collectibleMounts.begin(), _collectibleMounts.end(),
        [this](CollectibleMount const& m) { return m.spellId == _currentSession.targetMountSpellId; });

    if (it == _collectibleMounts.end())
        return;

    // Check if we have enough gold
    if (it->goldCost > 0 && GetBot()->GetMoney() < it->goldCost)
    {
        TC_LOG_DEBUG("module.playerbot.mounts",
            "MountCollectionManager: {} needs {} gold for mount, has {}",
            GetBot()->GetName(), it->goldCost, GetBot()->GetMoney());
        return;
    }

    // Navigate to vendor and purchase
    // Actual vendor interaction handled by VendorManager
}

bool MountCollectionManager::NavigateToFarmLocation()
{
    // Get mount data for target position
    auto it = std::find_if(_collectibleMounts.begin(), _collectibleMounts.end(),
        [this](CollectibleMount const& m) { return m.spellId == _currentSession.targetMountSpellId; });

    if (it == _collectibleMounts.end())
        return false;

    _currentSession.targetInstanceId = it->dropSourceInstanceId;
    _currentSession.isNavigating = true;

    // Actual navigation handled by MovementManager/NavigationManager
    return true;
}

void MountCollectionManager::CheckMountObtained()
{
    if (!_currentSession.isActive)
        return;

    // Check if mount was learned
    if (_ownedMountSpells.count(_currentSession.targetMountSpellId) > 0)
    {
        // Already knew it
        return;
    }

    // Check current mount list
    MountManager* mountMgr = GetMountManager();
    if (mountMgr && mountMgr->KnowsMount(_currentSession.targetMountSpellId))
    {
        // Mount obtained!
        _ownedMountSpells.insert(_currentSession.targetMountSpellId);
        _ownedMounts.fetch_add(1, std::memory_order_relaxed);
        _statistics.mountsObtained.fetch_add(1, std::memory_order_relaxed);
        _globalStatistics.mountsObtained.fetch_add(1, std::memory_order_relaxed);

        TC_LOG_INFO("module.playerbot.mounts",
            "MountCollectionManager: {} obtained mount {}!",
            GetBot()->GetName(), _currentSession.targetMountSpellId);

        NotifyCallback(_currentSession.targetMountSpellId, true);
        StopFarming("Mount obtained");
    }
}

std::vector<CollectibleMount> MountCollectionManager::GetObtainableMounts(MountSource source) const
{
    std::vector<CollectibleMount> result;

    for (auto const& mount : _collectibleMounts)
    {
        if (mount.isOwned)
            continue;

        if (source != MountSource::NONE && mount.source != source)
            continue;

        if (!IsSourceEnabled(mount.source))
            continue;

        result.push_back(mount);
    }

    return result;
}

std::vector<CollectibleMount> MountCollectionManager::GetMountsBySource(MountSource source) const
{
    return GetObtainableMounts(source);
}

std::vector<CollectibleMount> MountCollectionManager::GetMountsByRarity(MountRarity rarity) const
{
    std::vector<CollectibleMount> result;

    for (auto const& mount : _collectibleMounts)
    {
        if (!mount.isOwned && mount.rarity == rarity)
        {
            result.push_back(mount);
        }
    }

    return result;
}

std::vector<CollectibleMount> MountCollectionManager::GetRecommendedMounts(uint32 maxCount) const
{
    std::vector<CollectibleMount> result;

    for (auto const& mount : _collectibleMounts)
    {
        if (mount.isOwned)
            continue;

        if (!mount.isFarmable)
            continue;

        if (!IsSourceEnabled(mount.source))
            continue;

        if (mount.rarity > _maxRarity)
            continue;

        result.push_back(mount);

        if (result.size() >= maxCount)
            break;
    }

    return result;
}

bool MountCollectionManager::IsMountObtainable(uint32 spellId) const
{
    auto it = std::find_if(_collectibleMounts.begin(), _collectibleMounts.end(),
        [spellId](CollectibleMount const& m) { return m.spellId == spellId; });

    return it != _collectibleMounts.end() && it->isFarmable;
}

float MountCollectionManager::GetCollectionProgress() const
{
    if (_collectibleMounts.empty() && _ownedMounts.load() == 0)
        return 0.0f;

    uint32 total = _collectibleMounts.size() + _ownedMounts.load();
    if (total == 0)
        return 0.0f;

    return static_cast<float>(_ownedMounts.load()) / static_cast<float>(total);
}

bool MountCollectionManager::FarmMount(uint32 mountSpellId)
{
    // Validate mount
    auto it = std::find_if(_collectibleMounts.begin(), _collectibleMounts.end(),
        [mountSpellId](CollectibleMount const& m) { return m.spellId == mountSpellId; });

    if (it == _collectibleMounts.end())
    {
        TC_LOG_DEBUG("module.playerbot.mounts",
            "MountCollectionManager: Mount {} not in collectible list for {}",
            mountSpellId, GetBot() ? GetBot()->GetName() : "unknown");
        return false;
    }

    if (it->isOwned)
    {
        TC_LOG_DEBUG("module.playerbot.mounts",
            "MountCollectionManager: Mount {} already owned by {}",
            mountSpellId, GetBot() ? GetBot()->GetName() : "unknown");
        return false;
    }

    // Stop any current session
    if (_currentSession.isActive)
    {
        StopFarming("Starting new mount farm");
    }

    // Start new session
    _currentSession.Reset();
    _currentSession.targetMountSpellId = mountSpellId;
    _currentSession.source = it->source;
    _currentSession.startTime = std::chrono::steady_clock::now();
    _currentSession.isActive = true;

    TC_LOG_DEBUG("module.playerbot.mounts",
        "MountCollectionManager: {} started farming mount {} (source: {})",
        GetBot() ? GetBot()->GetName() : "unknown",
        mountSpellId, static_cast<uint8>(it->source));

    return true;
}

void MountCollectionManager::StopFarming(std::string const& reason)
{
    if (!_currentSession.isActive)
        return;

    _statistics.totalFarmTimeMs.fetch_add(_currentSession.GetElapsedMs(), std::memory_order_relaxed);
    _globalStatistics.totalFarmTimeMs.fetch_add(_currentSession.GetElapsedMs(), std::memory_order_relaxed);

    TC_LOG_DEBUG("module.playerbot.mounts",
        "MountCollectionManager: {} stopped farming mount {}, reason: {}, attempts: {}",
        GetBot() ? GetBot()->GetName() : "unknown",
        _currentSession.targetMountSpellId,
        reason.empty() ? "none" : reason.c_str(),
        _currentSession.attemptsThisSession);

    _currentSession.Reset();
}

bool MountCollectionManager::FarmRaidMounts()
{
    auto raidMounts = GetFarmableRaidMounts();
    if (raidMounts.empty())
    {
        TC_LOG_DEBUG("module.playerbot.mounts",
            "MountCollectionManager: No farmable raid mounts for {}",
            GetBot() ? GetBot()->GetName() : "unknown");
        return false;
    }

    // Select best raid mount to farm
    return FarmMount(raidMounts[0].spellId);
}

bool MountCollectionManager::FarmRepMounts(uint32 factionId)
{
    auto repMounts = GetFarmableRepMounts();
    if (repMounts.empty())
        return false;

    // Filter by faction if specified
    if (factionId > 0)
    {
        auto it = std::find_if(repMounts.begin(), repMounts.end(),
            [factionId](CollectibleMount const& m) { return m.requiredReputation == factionId; });
        if (it != repMounts.end())
        {
            return FarmMount(it->spellId);
        }
        return false;
    }

    // Select best reputation mount
    return FarmMount(repMounts[0].spellId);
}

bool MountCollectionManager::FarmAchievementMounts()
{
    auto achMounts = GetObtainableMounts(MountSource::ACHIEVEMENT);
    if (achMounts.empty())
        return false;

    return FarmMount(achMounts[0].spellId);
}

bool MountCollectionManager::FarmDungeonMounts()
{
    auto dungeonMounts = GetObtainableMounts(MountSource::DUNGEON_DROP);
    if (dungeonMounts.empty())
        return false;

    return FarmMount(dungeonMounts[0].spellId);
}

bool MountCollectionManager::AutoFarm()
{
    uint32 mountToFarm = SelectNextMountToFarm();
    if (mountToFarm == 0)
    {
        TC_LOG_DEBUG("module.playerbot.mounts",
            "MountCollectionManager: No mounts available for auto-farm for {}",
            GetBot() ? GetBot()->GetName() : "unknown");
        return false;
    }

    return FarmMount(mountToFarm);
}

std::vector<CollectibleMount> MountCollectionManager::GetFarmableRaidMounts() const
{
    std::vector<CollectibleMount> result;

    for (auto const& mount : _collectibleMounts)
    {
        if (mount.isOwned || mount.source != MountSource::RAID_DROP)
            continue;

        // Check lockout
        if (!IsRaidMountFarmable(mount.spellId))
            continue;

        // Prioritize legacy content
        if (_prioritizeLegacy && !mount.isLegacy)
            continue;

        result.push_back(mount);
    }

    // Sort by drop chance (higher first) for efficiency
    std::sort(result.begin(), result.end(),
        [](CollectibleMount const& a, CollectibleMount const& b) {
            return a.dropChance > b.dropChance;
        });

    return result;
}

bool MountCollectionManager::IsRaidMountFarmable(uint32 spellId) const
{
    // Check if we're on lockout for this mount's raid
    auto it = std::find_if(_collectibleMounts.begin(), _collectibleMounts.end(),
        [spellId](CollectibleMount const& m) { return m.spellId == spellId; });

    if (it == _collectibleMounts.end() || !it->isWeeklyLockout)
        return true;

    // Check instance lockout
    // TrinityCore 11.x uses InstanceLockMgr for lockout tracking
    return GetRaidLockoutReset(spellId) <= GameTime::GetGameTime();
}

time_t MountCollectionManager::GetRaidLockoutReset(uint32 spellId) const
{
    auto it = std::find_if(_collectibleMounts.begin(), _collectibleMounts.end(),
        [spellId](CollectibleMount const& m) { return m.spellId == spellId; });

    if (it == _collectibleMounts.end())
        return 0;

    // Would query InstanceLockMgr for actual lockout status
    // For now, return 0 (not locked)
    return 0;
}

std::vector<CollectibleMount> MountCollectionManager::GetFarmableRepMounts() const
{
    std::vector<CollectibleMount> result;

    for (auto const& mount : _collectibleMounts)
    {
        if (mount.isOwned || mount.source != MountSource::REPUTATION)
            continue;

        // Check if close to required reputation
        float progress = GetRepMountProgress(mount.spellId);
        if (progress >= 0.5f)  // At least 50% progress
        {
            result.push_back(mount);
        }
    }

    // Sort by progress (highest first)
    std::sort(result.begin(), result.end(),
        [this](CollectibleMount const& a, CollectibleMount const& b) {
            return GetRepMountProgress(a.spellId) > GetRepMountProgress(b.spellId);
        });

    return result;
}

float MountCollectionManager::GetRepMountProgress(uint32 spellId) const
{
    if (!GetBot())
        return 0.0f;

    auto it = std::find_if(_collectibleMounts.begin(), _collectibleMounts.end(),
        [spellId](CollectibleMount const& m) { return m.spellId == spellId; });

    if (it == _collectibleMounts.end() || it->requiredReputation == 0)
        return 0.0f;

    // Get faction entry for reputation check
    FactionEntry const* factionEntry = sFactionStore.LookupEntry(it->requiredReputation);
    if (!factionEntry)
        return 0.0f;

    // Get current reputation standing
    ReputationRank rank = GetBot()->GetReputationMgr().GetRank(factionEntry);
    uint32 requiredRank = it->requiredReputationStanding;

    if (static_cast<uint32>(rank) >= requiredRank)
        return 1.0f;

    // Calculate progress toward required rank
    return static_cast<float>(rank) / static_cast<float>(requiredRank);
}

void MountCollectionManager::SetSourceEnabled(MountSource source, bool enabled)
{
    if (enabled)
        _enabledSources.insert(source);
    else
        _enabledSources.erase(source);
}

bool MountCollectionManager::IsSourceEnabled(MountSource source) const
{
    return _enabledSources.count(source) > 0;
}

uint32 MountCollectionManager::SelectNextMountToFarm() const
{
    auto recommended = GetRecommendedMounts(1);
    if (!recommended.empty())
    {
        return recommended[0].spellId;
    }
    return 0;
}

MountManager* MountCollectionManager::GetMountManager() const
{
    // Would retrieve from GameSystemsManager
    // For now, return nullptr - actual integration done at higher level
    return nullptr;
}

void MountCollectionManager::LoadMountDatabase()
{
    // Load mount data from DB2/DBC
    // This populates the static _mountDatabase with all available mounts

    TC_LOG_INFO("module.playerbot.mounts",
        "MountCollectionManager: Loading mount database...");

    // Example mount entries - actual implementation would load from DB2
    // These represent common farmable mounts

    // Raid drop mounts (Legacy)
    {
        CollectibleMount mount;
        mount.spellId = 63796;  // Mimiron's Head
        mount.name = "Mimiron's Head";
        mount.source = MountSource::RAID_DROP;
        mount.rarity = MountRarity::LEGENDARY;
        mount.dropSourceEntry = 33288;  // Yogg-Saron
        mount.dropSourceInstanceId = 603;  // Ulduar
        mount.dropChance = 1.0f;
        mount.isWeeklyLockout = true;
        mount.isLegacy = true;
        mount.requiredLevel = 30;
        _mountDatabase[mount.spellId] = mount;
    }

    {
        CollectibleMount mount;
        mount.spellId = 72286;  // Invincible
        mount.name = "Invincible";
        mount.source = MountSource::RAID_DROP;
        mount.rarity = MountRarity::LEGENDARY;
        mount.dropSourceEntry = 36597;  // The Lich King
        mount.dropSourceInstanceId = 631;  // ICC
        mount.dropChance = 1.0f;
        mount.isWeeklyLockout = true;
        mount.isLegacy = true;
        mount.requiredLevel = 30;
        _mountDatabase[mount.spellId] = mount;
    }

    {
        CollectibleMount mount;
        mount.spellId = 97493;  // Pureblood Fire Hawk
        mount.name = "Pureblood Fire Hawk";
        mount.source = MountSource::RAID_DROP;
        mount.rarity = MountRarity::EPIC;
        mount.dropSourceEntry = 52409;  // Ragnaros
        mount.dropSourceInstanceId = 720;  // Firelands
        mount.dropChance = 1.5f;
        mount.isWeeklyLockout = true;
        mount.isLegacy = true;
        mount.requiredLevel = 35;
        _mountDatabase[mount.spellId] = mount;
    }

    {
        CollectibleMount mount;
        mount.spellId = 69395;  // Onyxian Drake
        mount.name = "Onyxian Drake";
        mount.source = MountSource::RAID_DROP;
        mount.rarity = MountRarity::EPIC;
        mount.dropSourceEntry = 10184;  // Onyxia
        mount.dropSourceInstanceId = 249;  // Onyxia's Lair
        mount.dropChance = 1.0f;
        mount.isWeeklyLockout = true;
        mount.isLegacy = true;
        mount.requiredLevel = 30;
        _mountDatabase[mount.spellId] = mount;
    }

    // Reputation mounts
    {
        CollectibleMount mount;
        mount.spellId = 32235;  // Golden Gryphon
        mount.name = "Golden Gryphon";
        mount.source = MountSource::REPUTATION;
        mount.rarity = MountRarity::COMMON;
        mount.requiredReputation = 72;  // Stormwind
        mount.requiredReputationStanding = REP_EXALTED;
        mount.goldCost = 100 * GOLD;
        mount.requiredLevel = 40;
        _mountDatabase[mount.spellId] = mount;
    }

    // Dungeon drop mounts
    {
        CollectibleMount mount;
        mount.spellId = 59961;  // Red Proto-Drake
        mount.name = "Red Proto-Drake";
        mount.source = MountSource::DUNGEON_DROP;
        mount.rarity = MountRarity::RARE;
        mount.dropSourceEntry = 26723;  // Skadi
        mount.dropSourceInstanceId = 575;  // Utgarde Pinnacle
        mount.dropChance = 1.0f;
        mount.isLegacy = true;
        mount.requiredLevel = 25;
        _mountDatabase[mount.spellId] = mount;
    }

    {
        CollectibleMount mount;
        mount.spellId = 63963;  // Blue Drake
        mount.name = "Blue Drake";
        mount.source = MountSource::DUNGEON_DROP;
        mount.rarity = MountRarity::RARE;
        mount.dropSourceEntry = 28859;  // Malygos
        mount.dropSourceInstanceId = 616;  // Eye of Eternity
        mount.dropChance = 4.0f;
        mount.isWeeklyLockout = true;
        mount.isLegacy = true;
        mount.requiredLevel = 30;
        _mountDatabase[mount.spellId] = mount;
    }

    // Achievement mounts
    {
        CollectibleMount mount;
        mount.spellId = 44153;  // Red Proto-Drake (Glory of the Hero)
        mount.name = "Red Proto-Drake (Achievement)";
        mount.source = MountSource::ACHIEVEMENT;
        mount.rarity = MountRarity::RARE;
        mount.requiredAchievement = 2136;  // Glory of the Hero
        mount.isLegacy = true;
        mount.requiredLevel = 25;
        _mountDatabase[mount.spellId] = mount;
    }

    TC_LOG_INFO("module.playerbot.mounts",
        "MountCollectionManager: Loaded {} mounts into database",
        _mountDatabase.size());
}

MountSource MountCollectionManager::ClassifyMountSource(uint32 /*spellId*/) const
{
    // Would analyze spell data to determine source
    return MountSource::UNKNOWN;
}

MountRarity MountCollectionManager::CalculateMountRarity(CollectibleMount const& mount) const
{
    // Based on source and drop chance
    if (mount.dropChance > 0 && mount.dropChance < 1.0f)
        return MountRarity::LEGENDARY;
    if (mount.dropChance >= 1.0f && mount.dropChance < 5.0f)
        return MountRarity::EPIC;
    if (mount.source == MountSource::ACHIEVEMENT)
        return MountRarity::RARE;
    if (mount.source == MountSource::REPUTATION)
        return MountRarity::UNCOMMON;
    return MountRarity::COMMON;
}

bool MountCollectionManager::MeetsMountRequirements(CollectibleMount const& mount) const
{
    if (!GetBot())
        return false;

    // Level requirement
    if (mount.requiredLevel > 0 && GetBot()->GetLevel() < mount.requiredLevel)
        return false;

    // Gold requirement
    if (mount.goldCost > 0 && GetBot()->GetMoney() < mount.goldCost)
        return false;

    // Achievement requirement
    if (mount.requiredAchievement > 0)
    {
        // Would check via Player::GetAchievementMgr()
        // For now, assume not complete
        return false;
    }

    return true;
}

void MountCollectionManager::NotifyCallback(uint32 mountSpellId, bool obtained)
{
    if (_callback)
    {
        _callback(mountSpellId, obtained);
    }
}

} // namespace Playerbot
