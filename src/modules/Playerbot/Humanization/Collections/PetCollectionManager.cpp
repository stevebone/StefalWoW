/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "PetCollectionManager.h"
#include "AI/BotAI.h"
#include "Player.h"
#include "Companion/BattlePetManager.h"
#include "DB2Stores.h"
#include "Log.h"
#include "World.h"
#include "GameTime.h"
#include "Map.h"
#include "AreaTrigger.h"
#include <algorithm>

namespace Playerbot
{

// Static member definitions
PetCollectionManager::CollectionStatistics PetCollectionManager::_globalStatistics;
std::unordered_map<uint32, CollectiblePet> PetCollectionManager::_petDatabase;
bool PetCollectionManager::_databaseLoaded = false;

PetCollectionManager::PetCollectionManager(Player* bot, BotAI* ai)
    : BehaviorManager(bot, ai, 10000, "PetCollectionManager")  // 10 second update
{
    // Enable all sources by default
    _enabledSources.insert(PetSource::WILD_CAPTURE);
    _enabledSources.insert(PetSource::VENDOR);
    _enabledSources.insert(PetSource::DROP);
    _enabledSources.insert(PetSource::QUEST);
    _enabledSources.insert(PetSource::ACHIEVEMENT);
    _enabledSources.insert(PetSource::PROFESSION);
    _enabledSources.insert(PetSource::WORLD_EVENT);
}

bool PetCollectionManager::OnInitialize()
{
    if (!GetBot() || !GetBot()->IsInWorld())
        return false;

    // Load static pet database once
    if (!_databaseLoaded)
    {
        LoadPetDatabase();
        _databaseLoaded = true;
    }

    // Analyze pets for this bot
    AnalyzePets();
    _lastAnalysis = std::chrono::steady_clock::now();

    TC_LOG_DEBUG("module.playerbot.pets",
        "PetCollectionManager: Initialized for {} with {} owned pets, {} collectible",
        GetBot()->GetName(), _ownedPets.load(), _collectiblePets.size());

    return true;
}

void PetCollectionManager::OnShutdown()
{
    if (_currentSession.isActive)
    {
        StopFarming("Shutdown");
    }

    _collectiblePets.clear();
    _ownedPetSpecies.clear();
}

void PetCollectionManager::OnUpdate(uint32 elapsed)
{
    if (!GetBot() || !GetBot()->IsInWorld())
        return;

    // Re-analyze pets periodically
    auto now = std::chrono::steady_clock::now();
    auto timeSinceAnalysis = std::chrono::duration_cast<std::chrono::milliseconds>(now - _lastAnalysis).count();
    if (timeSinceAnalysis >= ANALYSIS_INTERVAL_MS)
    {
        AnalyzePets();
        _lastAnalysis = now;
    }

    // Update farming session if active
    if (_currentSession.isActive)
    {
        UpdateFarmingSession(elapsed);
    }
}

void PetCollectionManager::AnalyzePets()
{
    if (!GetBot())
        return;

    // Clear and rebuild collectible list
    _collectiblePets.clear();
    _ownedPetSpecies.clear();

    // Get owned pets from BattlePetManager
    BattlePetManager* petMgr = GetBattlePetManager();
    if (petMgr)
    {
        auto playerPets = petMgr->GetPlayerPets();
        for (auto const& pet : playerPets)
        {
            _ownedPetSpecies.insert(pet.speciesId);
        }
        _ownedPets.store(static_cast<uint32>(_ownedPetSpecies.size()), std::memory_order_release);
    }

    // Build collectible pet list from database
    for (auto const& [speciesId, petData] : _petDatabase)
    {
        CollectiblePet pet = petData;
        pet.isOwned = _ownedPetSpecies.count(speciesId) > 0;

        // Check if pet is farmable by this bot
        pet.isFarmable = !pet.isOwned && MeetsPetRequirements(pet);

        // Only add unowned pets that are potentially obtainable
        if (!pet.isOwned && IsSourceEnabled(pet.source))
        {
            _collectiblePets.push_back(pet);
        }
    }

    // Sort by priority
    std::sort(_collectiblePets.begin(), _collectiblePets.end(),
        [](CollectiblePet const& a, CollectiblePet const& b) {
            return a.GetPriorityScore() > b.GetPriorityScore();
        });

    TC_LOG_DEBUG("module.playerbot.pets",
        "PetCollectionManager: {} analyzed pets, {} owned, {} collectible",
        GetBot()->GetName(), _ownedPets.load(), _collectiblePets.size());
}

void PetCollectionManager::UpdateFarmingSession(uint32 /*elapsed*/)
{
    if (!_currentSession.isActive)
        return;

    // Check if we obtained the target pet
    CheckPetObtained();

    // Check max duration
    if (_currentSession.GetElapsedMs() > MAX_FARM_DURATION_MS)
    {
        StopFarming("Max duration reached");
        return;
    }

    // Execute farming step based on source
    switch (_currentSession.source)
    {
        case PetSource::WILD_CAPTURE:
            ExecuteWildCaptureStep();
            break;
        case PetSource::VENDOR:
            ExecuteVendorStep();
            break;
        case PetSource::DROP:
            ExecuteDropFarmStep();
            break;
        default:
            break;
    }
}

void PetCollectionManager::ExecuteWildCaptureStep()
{
    // Search for wild pet spawn
    if (_currentSession.isSearching)
    {
        // Check if wild pet is spawned nearby
        if (IsWildPetSpawned(_currentSession.targetSpeciesId))
        {
            // Check quality if we're being picky
            if (_captureRareOnly)
            {
                uint8 quality = GetBestAvailableQuality(_currentSession.targetSpeciesId);
                if (quality < _minCaptureQuality)
                {
                    // Skip this spawn, wait for better quality
                    return;
                }
            }

            // Start battle
            StartWildBattle();
        }
        return;
    }

    // Navigate to spawn location
    if (!_currentSession.isNavigating)
    {
        if (!NavigateToWildPet())
        {
            StopFarming("Failed to navigate to wild pet spawn");
        }
    }
}

void PetCollectionManager::ExecuteVendorStep()
{
    // Find pet data
    auto it = std::find_if(_collectiblePets.begin(), _collectiblePets.end(),
        [this](CollectiblePet const& p) { return p.speciesId == _currentSession.targetSpeciesId; });

    if (it == _collectiblePets.end())
        return;

    // Check if we have enough gold
    if (it->goldCost > 0 && GetBot()->GetMoney() < it->goldCost)
    {
        TC_LOG_DEBUG("module.playerbot.pets",
            "PetCollectionManager: {} needs {} gold for pet, has {}",
            GetBot()->GetName(), it->goldCost, GetBot()->GetMoney());
        return;
    }

    // Check max gold spend
    if (it->goldCost > _maxGoldSpend)
    {
        TC_LOG_DEBUG("module.playerbot.pets",
            "PetCollectionManager: Pet cost {} exceeds max spend {} for {}",
            it->goldCost, _maxGoldSpend, GetBot()->GetName());
        StopFarming("Pet too expensive");
        return;
    }

    // Navigate to vendor and purchase
    // Actual vendor interaction handled by VendorManager
}

void PetCollectionManager::ExecuteDropFarmStep()
{
    // Similar to mount drop farming
    // Navigate to creature and kill for drop
}

bool PetCollectionManager::NavigateToWildPet()
{
    Position spawnPos = FindNearestWildPet(_currentSession.targetSpeciesId);
    if (spawnPos.GetPositionX() == 0 && spawnPos.GetPositionY() == 0)
    {
        return false;
    }

    _currentSession.targetPosition = spawnPos;
    _currentSession.isNavigating = true;

    // Actual navigation handled by MovementManager
    return true;
}

bool PetCollectionManager::StartWildBattle()
{
    BattlePetManager* petMgr = GetBattlePetManager();
    if (!petMgr)
        return false;

    _currentSession.isSearching = false;
    _statistics.captureAttempts.fetch_add(1, std::memory_order_relaxed);
    _globalStatistics.captureAttempts.fetch_add(1, std::memory_order_relaxed);

    // Battle pet combat handled by BattlePetManager
    return true;
}

void PetCollectionManager::CheckPetObtained()
{
    if (!_currentSession.isActive)
        return;

    // Check if pet was learned
    if (_ownedPetSpecies.count(_currentSession.targetSpeciesId) > 0)
    {
        // Already knew it
        return;
    }

    // Check current pet list
    BattlePetManager* petMgr = GetBattlePetManager();
    if (petMgr && petMgr->OwnsPet(_currentSession.targetSpeciesId))
    {
        // Pet obtained!
        _ownedPetSpecies.insert(_currentSession.targetSpeciesId);
        _ownedPets.fetch_add(1, std::memory_order_relaxed);
        _statistics.petsObtained.fetch_add(1, std::memory_order_relaxed);
        _globalStatistics.petsObtained.fetch_add(1, std::memory_order_relaxed);

        // Track source-specific stats
        switch (_currentSession.source)
        {
            case PetSource::WILD_CAPTURE:
                _statistics.wildCaptured.fetch_add(1, std::memory_order_relaxed);
                _globalStatistics.wildCaptured.fetch_add(1, std::memory_order_relaxed);
                break;
            case PetSource::VENDOR:
                _statistics.vendorPurchased.fetch_add(1, std::memory_order_relaxed);
                _globalStatistics.vendorPurchased.fetch_add(1, std::memory_order_relaxed);
                break;
            case PetSource::DROP:
                _statistics.dropObtained.fetch_add(1, std::memory_order_relaxed);
                _globalStatistics.dropObtained.fetch_add(1, std::memory_order_relaxed);
                break;
            default:
                break;
        }

        TC_LOG_INFO("module.playerbot.pets",
            "PetCollectionManager: {} obtained pet {}!",
            GetBot()->GetName(), _currentSession.targetSpeciesId);

        NotifyCallback(_currentSession.targetSpeciesId, true);
        StopFarming("Pet obtained");
    }
}

std::vector<CollectiblePet> PetCollectionManager::GetCollectiblePets(PetSource source) const
{
    std::vector<CollectiblePet> result;

    for (auto const& pet : _collectiblePets)
    {
        if (pet.isOwned)
            continue;

        if (source != PetSource::NONE && pet.source != source)
            continue;

        if (!IsSourceEnabled(pet.source))
            continue;

        result.push_back(pet);
    }

    return result;
}

std::vector<CollectiblePet> PetCollectionManager::GetPetsBySource(PetSource source) const
{
    return GetCollectiblePets(source);
}

std::vector<CollectiblePet> PetCollectionManager::GetPetsByZone(uint32 zoneId) const
{
    std::vector<CollectiblePet> result;

    for (auto const& pet : _collectiblePets)
    {
        if (!pet.isOwned && pet.zoneId == zoneId)
        {
            result.push_back(pet);
        }
    }

    return result;
}

std::vector<CollectiblePet> PetCollectionManager::GetRecommendedPets(uint32 maxCount) const
{
    std::vector<CollectiblePet> result;

    for (auto const& pet : _collectiblePets)
    {
        if (pet.isOwned)
            continue;

        if (!pet.isFarmable)
            continue;

        if (!IsSourceEnabled(pet.source))
            continue;

        result.push_back(pet);

        if (result.size() >= maxCount)
            break;
    }

    return result;
}

bool PetCollectionManager::IsPetObtainable(uint32 speciesId) const
{
    auto it = std::find_if(_collectiblePets.begin(), _collectiblePets.end(),
        [speciesId](CollectiblePet const& p) { return p.speciesId == speciesId; });

    return it != _collectiblePets.end() && it->isFarmable;
}

float PetCollectionManager::GetCollectionProgress() const
{
    if (_collectiblePets.empty() && _ownedPets.load() == 0)
        return 0.0f;

    uint32 total = _collectiblePets.size() + _ownedPets.load();
    if (total == 0)
        return 0.0f;

    return static_cast<float>(_ownedPets.load()) / static_cast<float>(total);
}

std::vector<CollectiblePet> PetCollectionManager::GetWildPetsInCurrentZone() const
{
    if (!GetBot())
        return {};

    uint32 currentZone = GetBot()->GetZoneId();
    return GetPetsByZone(currentZone);
}

bool PetCollectionManager::FarmPet(uint32 speciesId)
{
    // Validate pet
    auto it = std::find_if(_collectiblePets.begin(), _collectiblePets.end(),
        [speciesId](CollectiblePet const& p) { return p.speciesId == speciesId; });

    if (it == _collectiblePets.end())
    {
        TC_LOG_DEBUG("module.playerbot.pets",
            "PetCollectionManager: Pet {} not in collectible list for {}",
            speciesId, GetBot() ? GetBot()->GetName() : "unknown");
        return false;
    }

    if (it->isOwned)
    {
        TC_LOG_DEBUG("module.playerbot.pets",
            "PetCollectionManager: Pet {} already owned by {}",
            speciesId, GetBot() ? GetBot()->GetName() : "unknown");
        return false;
    }

    // Stop any current session
    if (_currentSession.isActive)
    {
        StopFarming("Starting new pet farm");
    }

    // Start new session
    _currentSession.Reset();
    _currentSession.targetSpeciesId = speciesId;
    _currentSession.source = it->source;
    _currentSession.targetZoneId = it->zoneId;
    _currentSession.startTime = std::chrono::steady_clock::now();
    _currentSession.isActive = true;

    // For wild captures, we start in searching mode
    if (it->source == PetSource::WILD_CAPTURE)
    {
        _currentSession.isSearching = true;
    }

    TC_LOG_DEBUG("module.playerbot.pets",
        "PetCollectionManager: {} started farming pet {} (source: {})",
        GetBot() ? GetBot()->GetName() : "unknown",
        speciesId, static_cast<uint8>(it->source));

    return true;
}

void PetCollectionManager::StopFarming(std::string const& reason)
{
    if (!_currentSession.isActive)
        return;

    _statistics.totalFarmTimeMs.fetch_add(_currentSession.GetElapsedMs(), std::memory_order_relaxed);
    _globalStatistics.totalFarmTimeMs.fetch_add(_currentSession.GetElapsedMs(), std::memory_order_relaxed);

    TC_LOG_DEBUG("module.playerbot.pets",
        "PetCollectionManager: {} stopped farming pet {}, reason: {}, attempts: {}",
        GetBot() ? GetBot()->GetName() : "unknown",
        _currentSession.targetSpeciesId,
        reason.empty() ? "none" : reason.c_str(),
        _currentSession.attemptsThisSession);

    _currentSession.Reset();
}

bool PetCollectionManager::StartCollection()
{
    return AutoFarm();
}

bool PetCollectionManager::CaptureWildPets(uint32 zoneId)
{
    uint32 targetZone = zoneId;
    if (targetZone == 0 && GetBot())
    {
        targetZone = GetBot()->GetZoneId();
    }

    auto wildPets = GetPetsByZone(targetZone);
    std::vector<CollectiblePet> capturablePets;

    // Filter to wild capture source only
    for (auto const& pet : wildPets)
    {
        if (pet.source == PetSource::WILD_CAPTURE && !pet.isOwned)
        {
            capturablePets.push_back(pet);
        }
    }

    if (capturablePets.empty())
    {
        TC_LOG_DEBUG("module.playerbot.pets",
            "PetCollectionManager: No wild pets to capture in zone {} for {}",
            targetZone, GetBot() ? GetBot()->GetName() : "unknown");
        return false;
    }

    // Start farming the highest priority wild pet
    return FarmPet(capturablePets[0].speciesId);
}

bool PetCollectionManager::BuyVendorPets()
{
    auto vendorPets = GetAffordableVendorPets();
    if (vendorPets.empty())
    {
        TC_LOG_DEBUG("module.playerbot.pets",
            "PetCollectionManager: No affordable vendor pets for {}",
            GetBot() ? GetBot()->GetName() : "unknown");
        return false;
    }

    return FarmPet(vendorPets[0].speciesId);
}

bool PetCollectionManager::FarmDropPets()
{
    auto dropPets = GetCollectiblePets(PetSource::DROP);
    if (dropPets.empty())
        return false;

    return FarmPet(dropPets[0].speciesId);
}

bool PetCollectionManager::AutoFarm()
{
    uint32 petToFarm = SelectNextPetToFarm();
    if (petToFarm == 0)
    {
        TC_LOG_DEBUG("module.playerbot.pets",
            "PetCollectionManager: No pets available for auto-farm for {}",
            GetBot() ? GetBot()->GetName() : "unknown");
        return false;
    }

    return FarmPet(petToFarm);
}

std::vector<CollectiblePet> PetCollectionManager::GetWildPets(uint32 zoneId) const
{
    std::vector<CollectiblePet> result;

    for (auto const& pet : _collectiblePets)
    {
        if (pet.isOwned || pet.source != PetSource::WILD_CAPTURE)
            continue;

        if (zoneId > 0 && pet.zoneId != zoneId)
            continue;

        result.push_back(pet);
    }

    return result;
}

Position PetCollectionManager::FindNearestWildPet(uint32 speciesId) const
{
    if (!GetBot())
        return Position();

    Position botPos = GetBot()->GetPosition();
    Position nearest;
    float nearestDist = std::numeric_limits<float>::max();

    for (auto const& pet : _collectiblePets)
    {
        if (pet.source != PetSource::WILD_CAPTURE)
            continue;

        if (speciesId > 0 && pet.speciesId != speciesId)
            continue;

        for (auto const& spawn : pet.spawnPoints)
        {
            float dist = botPos.GetExactDist(&spawn);
            if (dist < nearestDist)
            {
                nearestDist = dist;
                nearest = spawn;
            }
        }
    }

    return nearest;
}

bool PetCollectionManager::IsWildPetSpawned(uint32 /*speciesId*/) const
{
    // Would search for nearby wild pet creatures
    // For now, return false - actual implementation uses creature grid search
    return false;
}

uint8 PetCollectionManager::GetBestAvailableQuality(uint32 /*speciesId*/) const
{
    // Would check quality of nearby spawned pets
    // For now, return common quality
    return 1;  // COMMON
}

std::vector<CollectiblePet> PetCollectionManager::GetVendorPets() const
{
    return GetCollectiblePets(PetSource::VENDOR);
}

std::vector<CollectiblePet> PetCollectionManager::GetAffordableVendorPets() const
{
    std::vector<CollectiblePet> result;

    if (!GetBot())
        return result;

    uint64 currentGold = GetBot()->GetMoney();

    for (auto const& pet : _collectiblePets)
    {
        if (pet.isOwned || pet.source != PetSource::VENDOR)
            continue;

        if (pet.goldCost <= currentGold && pet.goldCost <= _maxGoldSpend)
        {
            result.push_back(pet);
        }
    }

    // Sort by cost (cheapest first)
    std::sort(result.begin(), result.end(),
        [](CollectiblePet const& a, CollectiblePet const& b) {
            return a.goldCost < b.goldCost;
        });

    return result;
}

uint64 PetCollectionManager::GetTotalVendorPetCost() const
{
    uint64 total = 0;

    for (auto const& pet : _collectiblePets)
    {
        if (!pet.isOwned && pet.source == PetSource::VENDOR)
        {
            total += pet.goldCost;
        }
    }

    return total;
}

void PetCollectionManager::SetSourceEnabled(PetSource source, bool enabled)
{
    if (enabled)
        _enabledSources.insert(source);
    else
        _enabledSources.erase(source);
}

bool PetCollectionManager::IsSourceEnabled(PetSource source) const
{
    return _enabledSources.count(source) > 0;
}

uint32 PetCollectionManager::SelectNextPetToFarm() const
{
    auto recommended = GetRecommendedPets(1);
    if (!recommended.empty())
    {
        return recommended[0].speciesId;
    }
    return 0;
}

BattlePetManager* PetCollectionManager::GetBattlePetManager() const
{
    // Would retrieve from GameSystemsManager
    // For now, return nullptr - actual integration done at higher level
    return nullptr;
}

void PetCollectionManager::LoadPetDatabase()
{
    // Load pet data from DB2/DBC
    // This populates the static _petDatabase with all available pets

    TC_LOG_INFO("module.playerbot.pets",
        "PetCollectionManager: Loading pet database...");

    // Example pet entries - actual implementation would load from DB2
    // These represent common collectible pets

    // Vendor pets
    {
        CollectiblePet pet;
        pet.speciesId = 39;     // Mechanical Squirrel
        pet.name = "Mechanical Squirrel";
        pet.source = PetSource::VENDOR;
        pet.rarity = PetRarity::COMMON;
        pet.petFamily = 10;     // Mechanical
        pet.goldCost = 50 * SILVER;
        pet.requiredLevel = 1;
        _petDatabase[pet.speciesId] = pet;
    }

    {
        CollectiblePet pet;
        pet.speciesId = 55;     // Bombay Cat
        pet.name = "Bombay Cat";
        pet.source = PetSource::VENDOR;
        pet.rarity = PetRarity::COMMON;
        pet.petFamily = 8;      // Beast
        pet.goldCost = 40 * SILVER;
        pet.requiredLevel = 1;
        _petDatabase[pet.speciesId] = pet;
    }

    {
        CollectiblePet pet;
        pet.speciesId = 68;     // Great Horned Owl
        pet.name = "Great Horned Owl";
        pet.source = PetSource::VENDOR;
        pet.rarity = PetRarity::COMMON;
        pet.petFamily = 3;      // Flying
        pet.goldCost = 50 * SILVER;
        pet.requiredLevel = 1;
        _petDatabase[pet.speciesId] = pet;
    }

    // Wild capture pets
    {
        CollectiblePet pet;
        pet.speciesId = 379;    // Squirrel
        pet.name = "Squirrel";
        pet.source = PetSource::WILD_CAPTURE;
        pet.rarity = PetRarity::COMMON;
        pet.petFamily = 5;      // Critter
        pet.zoneId = 12;        // Elwynn Forest
        pet.spawnChance = 100.0f;
        pet.captureMinLevel = 1;
        pet.captureMaxLevel = 2;
        pet.spawnPoints.push_back(Position(-9465.0f, 97.0f, 58.0f, 0.0f));
        pet.spawnPoints.push_back(Position(-9458.0f, 45.0f, 57.0f, 0.0f));
        _petDatabase[pet.speciesId] = pet;
    }

    {
        CollectiblePet pet;
        pet.speciesId = 378;    // Rabbit
        pet.name = "Rabbit";
        pet.source = PetSource::WILD_CAPTURE;
        pet.rarity = PetRarity::COMMON;
        pet.petFamily = 5;      // Critter
        pet.zoneId = 12;        // Elwynn Forest
        pet.spawnChance = 100.0f;
        pet.captureMinLevel = 1;
        pet.captureMaxLevel = 2;
        pet.spawnPoints.push_back(Position(-9470.0f, 100.0f, 58.0f, 0.0f));
        _petDatabase[pet.speciesId] = pet;
    }

    {
        CollectiblePet pet;
        pet.speciesId = 417;    // Chicken
        pet.name = "Chicken";
        pet.source = PetSource::WILD_CAPTURE;
        pet.rarity = PetRarity::COMMON;
        pet.petFamily = 3;      // Flying
        pet.zoneId = 14;        // Westfall
        pet.spawnChance = 90.0f;
        pet.captureMinLevel = 1;
        pet.captureMaxLevel = 3;
        _petDatabase[pet.speciesId] = pet;
    }

    // Rare spawn pets
    {
        CollectiblePet pet;
        pet.speciesId = 1125;   // Stunted Direhorn
        pet.name = "Stunted Direhorn";
        pet.source = PetSource::WILD_CAPTURE;
        pet.rarity = PetRarity::RARE;
        pet.petFamily = 8;      // Beast
        pet.zoneId = 6507;      // Isle of Giants
        pet.spawnChance = 10.0f;
        pet.isRareSpawn = true;
        pet.captureMinLevel = 23;
        pet.captureMaxLevel = 25;
        _petDatabase[pet.speciesId] = pet;
    }

    // Drop pets
    {
        CollectiblePet pet;
        pet.speciesId = 1168;   // Filthling
        pet.name = "Filthling";
        pet.source = PetSource::DROP;
        pet.rarity = PetRarity::RARE;
        pet.petFamily = 7;      // Elemental
        pet.dropSourceEntry = 69251; // Quivering Filth
        pet.dropChance = 15.0f;
        pet.requiredLevel = 90;
        _petDatabase[pet.speciesId] = pet;
    }

    // Achievement pets
    {
        CollectiblePet pet;
        pet.speciesId = 847;    // Nuts (1000 pets achievement)
        pet.name = "Nuts";
        pet.source = PetSource::ACHIEVEMENT;
        pet.rarity = PetRarity::EPIC;
        pet.petFamily = 5;      // Critter
        pet.requiredAchievement = 9643;  // So. Many. Pets.
        _petDatabase[pet.speciesId] = pet;
    }

    TC_LOG_INFO("module.playerbot.pets",
        "PetCollectionManager: Loaded {} pets into database",
        _petDatabase.size());
}

PetSource PetCollectionManager::ClassifyPetSource(uint32 /*speciesId*/) const
{
    // Would analyze pet data to determine source
    return PetSource::UNKNOWN;
}

PetRarity PetCollectionManager::CalculatePetRarity(CollectiblePet const& pet) const
{
    // Based on source and spawn chance
    if (pet.isRareSpawn || pet.spawnChance < 10.0f)
        return PetRarity::EPIC;
    if (pet.source == PetSource::DROP && pet.dropChance < 10.0f)
        return PetRarity::RARE;
    if (pet.source == PetSource::ACHIEVEMENT)
        return PetRarity::RARE;
    if (pet.source == PetSource::VENDOR && pet.goldCost > 100 * GOLD)
        return PetRarity::UNCOMMON;
    return PetRarity::COMMON;
}

bool PetCollectionManager::MeetsPetRequirements(CollectiblePet const& pet) const
{
    if (!GetBot())
        return false;

    // Level requirement
    if (pet.requiredLevel > 0 && GetBot()->GetLevel() < pet.requiredLevel)
        return false;

    // Gold requirement (rough check - actual affordability checked later)
    if (pet.goldCost > 0 && GetBot()->GetMoney() < pet.goldCost / 2)
        return false;

    // Achievement requirement
    if (pet.requiredAchievement > 0)
    {
        // Would check via Player::GetAchievementMgr()
        // For now, assume not complete
        return false;
    }

    return true;
}

void PetCollectionManager::NotifyCallback(uint32 speciesId, bool obtained)
{
    if (_callback)
    {
        _callback(speciesId, obtained);
    }
}

} // namespace Playerbot
