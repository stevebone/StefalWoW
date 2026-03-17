/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * PET COLLECTION MANAGER
 *
 * Phase 3: Humanization Core (GOD_TIER Task 8)
 *
 * Manages pet collection and farming for bots:
 * - Identifies collectible pets for bot's level/zone
 * - Farms specific pets (wild captures, vendor, drops)
 * - Tracks collection progress and priorities
 * - Coordinates with BattlePetManager for battles/captures
 */

#pragma once

#include "Define.h"
#include "SharedDefines.h"
#include "AI/BehaviorManager.h"
#include "ObjectGuid.h"
#include "Position.h"
#include <atomic>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include <functional>
#include <memory>

class Player;

namespace Playerbot
{

class BotAI;
class BattlePetManager;

/**
 * @enum PetSource
 * @brief Source/method to obtain a battle pet
 */
enum class PetSource : uint8
{
    NONE = 0,
    WILD_CAPTURE,       // Captured in the wild (pet battle)
    VENDOR,             // Purchasable from vendor
    DROP,               // Drops from creatures/bosses
    QUEST,              // Quest reward
    ACHIEVEMENT,        // Achievement reward
    PROFESSION,         // Crafted via profession
    WORLD_EVENT,        // Holiday/world event reward
    PROMOTION,          // Promotional (collector's edition)
    STORE,              // In-game store
    TRADING_POST,       // Trading Post reward
    PET_CHARM,          // Purchased with pet charms
    GARRISON,           // Garrison pet
    UNKNOWN
};

/**
 * @enum PetRarity
 * @brief Rarity classification for prioritization
 */
enum class PetRarity : uint8
{
    COMMON = 0,         // Easy to obtain
    UNCOMMON,           // Some effort required
    RARE,               // Significant effort
    EPIC,               // Major time investment
    LEGENDARY           // Extremely rare/difficult
};

/**
 * @struct CollectiblePet
 * @brief Information about a pet that can be collected
 */
struct CollectiblePet
{
    uint32 speciesId{0};            // Pet species ID
    uint32 creatureId{0};           // Creature ID for wild pets
    uint32 itemId{0};               // Item that teaches pet (if applicable)
    std::string name;
    PetSource source{PetSource::UNKNOWN};
    PetRarity rarity{PetRarity::COMMON};

    // Pet battle info
    uint8 petFamily{0};             // Pet family type
    uint8 petQuality{0};            // Default quality (0-5)
    bool canBattle{true};           // Can participate in battles

    // Requirements
    uint8 requiredLevel{0};
    uint32 requiredReputation{0};
    uint32 requiredAchievement{0};
    uint32 requiredQuest{0};
    uint64 goldCost{0};             // Gold cost in copper
    uint32 petCharmCost{0};         // Pet charm cost

    // Wild capture info
    uint32 zoneId{0};               // Zone where pet spawns
    std::vector<Position> spawnPoints; // Known spawn locations
    float spawnChance{100.0f};      // Spawn probability
    bool isRareSpawn{false};        // Rare/limited spawns
    uint8 captureMinLevel{1};       // Min level to capture
    uint8 captureMaxLevel{25};      // Max level to capture

    // Drop info
    uint32 dropSourceEntry{0};      // Creature that drops pet
    uint32 dropInstanceId{0};       // Instance ID if applicable
    float dropChance{0.0f};         // Drop chance (0-100)

    // State
    bool isOwned{false};            // Already owned by bot
    bool isFarmable{false};         // Can bot currently farm this
    uint32 farmAttempts{0};         // Number of farm attempts

    /**
     * @brief Calculate farming priority score
     * @return Priority score (higher = farm sooner)
     */
    float GetPriorityScore() const
    {
        float score = 100.0f;

        // Reduce score based on rarity
        score -= static_cast<float>(rarity) * 15.0f;

        // Boost wild captures (easy to get)
        if (source == PetSource::WILD_CAPTURE)
            score += 25.0f;

        // Boost vendor pets (guaranteed)
        if (source == PetSource::VENDOR && goldCost < 1000 * GOLD)
            score += 20.0f;

        // Boost higher spawn/drop chances
        if (source == PetSource::WILD_CAPTURE)
            score += spawnChance * 0.3f;
        else if (dropChance > 0)
            score += dropChance * 0.5f;

        // Reduce score for rare spawns
        if (isRareSpawn)
            score -= 10.0f;

        return score;
    }
};

/**
 * @struct PetFarmSession
 * @brief Tracks an active pet farming session
 */
struct PetFarmSession
{
    uint32 targetSpeciesId{0};
    PetSource source{PetSource::NONE};
    std::chrono::steady_clock::time_point startTime;
    uint32 attemptsThisSession{0};
    uint32 capturesThisSession{0};
    bool isActive{false};

    // Navigation state
    Position targetPosition;
    uint32 targetZoneId{0};
    bool isNavigating{false};
    bool isSearching{false};      // Searching for wild pet spawn

    void Reset()
    {
        targetSpeciesId = 0;
        source = PetSource::NONE;
        attemptsThisSession = 0;
        capturesThisSession = 0;
        isActive = false;
        isNavigating = false;
        isSearching = false;
    }

    uint32 GetElapsedMs() const
    {
        if (!isActive)
            return 0;
        auto now = std::chrono::steady_clock::now();
        return static_cast<uint32>(
            std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count());
    }
};

/**
 * @brief Callback for pet collection events
 */
using PetCollectionCallback = std::function<void(uint32 speciesId, bool obtained)>;

/**
 * @class PetCollectionManager
 * @brief Manages pet collection and farming for bots
 *
 * This manager:
 * - Analyzes available pets based on bot capabilities
 * - Prioritizes pets by obtainability and rarity
 * - Executes farming strategies (wild capture, vendor, drops)
 * - Coordinates with BattlePetManager for battle captures
 *
 * Update interval: 10000ms (10 seconds)
 */
class TC_GAME_API PetCollectionManager : public BehaviorManager
{
public:
    explicit PetCollectionManager(Player* bot, BotAI* ai);
    ~PetCollectionManager() override = default;

    // ========================================================================
    // FAST STATE QUERIES
    // ========================================================================

    /**
     * @brief Check if bot is actively farming pets
     */
    bool IsFarming() const { return _currentSession.isActive; }

    /**
     * @brief Get current farming target
     */
    uint32 GetCurrentTarget() const { return _currentSession.targetSpeciesId; }

    /**
     * @brief Get total pets owned
     */
    uint32 GetOwnedPetCount() const { return _ownedPets.load(std::memory_order_acquire); }

    /**
     * @brief Get total collectible pets discovered
     */
    uint32 GetCollectibleCount() const { return static_cast<uint32>(_collectiblePets.size()); }

    /**
     * @brief Check if currently searching for wild pets
     */
    bool IsSearchingWild() const { return _currentSession.isSearching; }

    // ========================================================================
    // COLLECTION ANALYSIS
    // ========================================================================

    /**
     * @brief Get all collectible pets for this bot
     * @param source Filter by source (NONE = all sources)
     * @return Vector of collectible pets sorted by priority
     */
    std::vector<CollectiblePet> GetCollectiblePets(
        PetSource source = PetSource::NONE) const;

    /**
     * @brief Get pets by source type
     * @param source Pet source to filter by
     * @return Vector of pets from specified source
     */
    std::vector<CollectiblePet> GetPetsBySource(PetSource source) const;

    /**
     * @brief Get pets by zone
     * @param zoneId Zone ID
     * @return Vector of pets available in zone
     */
    std::vector<CollectiblePet> GetPetsByZone(uint32 zoneId) const;

    /**
     * @brief Get recommended pets to farm
     * @param maxCount Maximum number of recommendations
     * @return Vector of recommended pets sorted by priority
     */
    std::vector<CollectiblePet> GetRecommendedPets(uint32 maxCount = 10) const;

    /**
     * @brief Check if specific pet is obtainable
     * @param speciesId Pet species ID
     * @return true if bot can obtain this pet
     */
    bool IsPetObtainable(uint32 speciesId) const;

    /**
     * @brief Get collection completion percentage
     * @return Completion percentage (0.0 to 1.0)
     */
    float GetCollectionProgress() const;

    /**
     * @brief Get wild pets available in current zone
     * @return Vector of wild pets in current zone
     */
    std::vector<CollectiblePet> GetWildPetsInCurrentZone() const;

    // ========================================================================
    // FARMING CONTROL
    // ========================================================================

    /**
     * @brief Start farming a specific pet
     * @param speciesId Pet species ID to farm
     * @return true if farming started
     */
    bool FarmPet(uint32 speciesId);

    /**
     * @brief Stop current farming session
     * @param reason Reason for stopping
     */
    void StopFarming(std::string const& reason = "");

    /**
     * @brief Start general collection mode
     * @return true if collection started
     */
    bool StartCollection();

    /**
     * @brief Start capturing wild pets in current zone
     * @param zoneId Specific zone (0 = current zone)
     * @return true if wild capture started
     */
    bool CaptureWildPets(uint32 zoneId = 0);

    /**
     * @brief Buy available vendor pets
     * @return true if vendor purchases started
     */
    bool BuyVendorPets();

    /**
     * @brief Farm pets that drop from creatures
     * @return true if drop farming started
     */
    bool FarmDropPets();

    /**
     * @brief Auto-select and farm best available pet
     * @return true if auto-farm started
     */
    bool AutoFarm();

    /**
     * @brief Get current farming session info
     */
    PetFarmSession const& GetCurrentSession() const { return _currentSession; }

    // ========================================================================
    // WILD PET CAPTURE
    // ========================================================================

    /**
     * @brief Get list of wild pets to capture
     * @param zoneId Zone filter (0 = all zones)
     * @return Vector of wild capture pets
     */
    std::vector<CollectiblePet> GetWildPets(uint32 zoneId = 0) const;

    /**
     * @brief Find nearest wild pet spawn location
     * @param speciesId Specific species (0 = any)
     * @return Position of nearest spawn or empty position
     */
    Position FindNearestWildPet(uint32 speciesId = 0) const;

    /**
     * @brief Check if wild pet is currently spawned
     * @param speciesId Pet species to check
     * @return true if pet is spawned nearby
     */
    bool IsWildPetSpawned(uint32 speciesId) const;

    /**
     * @brief Get best quality pet of species to capture
     * @param speciesId Pet species
     * @return Quality level (0-5, or 255 if none found)
     */
    uint8 GetBestAvailableQuality(uint32 speciesId) const;

    // ========================================================================
    // VENDOR PETS
    // ========================================================================

    /**
     * @brief Get list of purchasable vendor pets
     * @return Vector of vendor pets
     */
    std::vector<CollectiblePet> GetVendorPets() const;

    /**
     * @brief Get vendor pets we can afford
     * @return Vector of affordable vendor pets
     */
    std::vector<CollectiblePet> GetAffordableVendorPets() const;

    /**
     * @brief Calculate total cost of all vendor pets
     * @return Total gold cost in copper
     */
    uint64 GetTotalVendorPetCost() const;

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    /**
     * @brief Set whether to capture rare quality only
     */
    void SetCaptureRareOnly(bool enable) { _captureRareOnly = enable; }

    /**
     * @brief Set maximum gold to spend on vendor pets
     */
    void SetMaxGoldSpend(uint64 gold) { _maxGoldSpend = gold; }

    /**
     * @brief Set callback for pet events
     */
    void SetCallback(PetCollectionCallback callback) { _callback = std::move(callback); }

    /**
     * @brief Enable/disable specific source types
     */
    void SetSourceEnabled(PetSource source, bool enabled);

    /**
     * @brief Check if source type is enabled
     */
    bool IsSourceEnabled(PetSource source) const;

    /**
     * @brief Set minimum quality to capture
     */
    void SetMinCaptureQuality(uint8 quality) { _minCaptureQuality = quality; }

    // ========================================================================
    // STATISTICS
    // ========================================================================

    struct CollectionStatistics
    {
        std::atomic<uint32> petsObtained{0};
        std::atomic<uint32> wildCaptured{0};
        std::atomic<uint32> vendorPurchased{0};
        std::atomic<uint32> dropObtained{0};
        std::atomic<uint32> battlesWon{0};
        std::atomic<uint32> captureAttempts{0};
        std::atomic<uint64> totalFarmTimeMs{0};
        std::atomic<uint64> goldSpent{0};

        void Reset()
        {
            petsObtained = 0;
            wildCaptured = 0;
            vendorPurchased = 0;
            dropObtained = 0;
            battlesWon = 0;
            captureAttempts = 0;
            totalFarmTimeMs = 0;
            goldSpent = 0;
        }
    };

    CollectionStatistics const& GetStatistics() const { return _statistics; }
    static CollectionStatistics const& GetGlobalStatistics() { return _globalStatistics; }

protected:
    // ========================================================================
    // BEHAVIOR MANAGER INTERFACE
    // ========================================================================

    void OnUpdate(uint32 elapsed);
    bool OnInitialize();
    void OnShutdown();

private:
    // ========================================================================
    // INTERNAL METHODS
    // ========================================================================

    /**
     * @brief Analyze all pets and build collection cache
     */
    void AnalyzePets();

    /**
     * @brief Update farming session state
     */
    void UpdateFarmingSession(uint32 elapsed);

    /**
     * @brief Execute wild capture farming step
     */
    void ExecuteWildCaptureStep();

    /**
     * @brief Execute vendor purchase step
     */
    void ExecuteVendorStep();

    /**
     * @brief Execute drop farming step
     */
    void ExecuteDropFarmStep();

    /**
     * @brief Navigate to wild pet spawn location
     * @return true if navigation in progress
     */
    bool NavigateToWildPet();

    /**
     * @brief Initiate pet battle with wild pet
     * @return true if battle started
     */
    bool StartWildBattle();

    /**
     * @brief Check if bot obtained target pet
     */
    void CheckPetObtained();

    /**
     * @brief Select next pet to farm based on priority
     * @return Pet species ID or 0 if none available
     */
    uint32 SelectNextPetToFarm() const;

    /**
     * @brief Get BattlePetManager reference
     */
    BattlePetManager* GetBattlePetManager() const;

    /**
     * @brief Load pet database from DBC/DB2
     */
    void LoadPetDatabase();

    /**
     * @brief Classify pet source from data
     */
    PetSource ClassifyPetSource(uint32 speciesId) const;

    /**
     * @brief Calculate pet rarity
     */
    PetRarity CalculatePetRarity(CollectiblePet const& pet) const;

    /**
     * @brief Check if pet meets requirements
     */
    bool MeetsPetRequirements(CollectiblePet const& pet) const;

    /**
     * @brief Notify callback of pet event
     */
    void NotifyCallback(uint32 speciesId, bool obtained);

    // ========================================================================
    // DATA
    // ========================================================================

    // Session state
    PetFarmSession _currentSession;

    // Configuration
    bool _captureRareOnly{false};
    uint64 _maxGoldSpend{1000 * GOLD};  // 1000 gold max spend
    uint8 _minCaptureQuality{2};        // Minimum uncommon quality
    std::unordered_set<PetSource> _enabledSources;

    // Collection data
    std::vector<CollectiblePet> _collectiblePets;
    std::unordered_set<uint32> _ownedPetSpecies;
    std::atomic<uint32> _ownedPets{0};

    // Cache
    std::chrono::steady_clock::time_point _lastAnalysis;

    // Callback
    PetCollectionCallback _callback;

    // Statistics
    CollectionStatistics _statistics;
    static CollectionStatistics _globalStatistics;

    // Static pet database (shared across all bots)
    static std::unordered_map<uint32, CollectiblePet> _petDatabase;
    static bool _databaseLoaded;

    // Constants
    static constexpr uint32 ANALYSIS_INTERVAL_MS = 300000;  // 5 minutes
    static constexpr uint32 MAX_FARM_DURATION_MS = 3600000; // 1 hour max per session
    static constexpr uint32 WILD_SEARCH_RADIUS = 100;       // Yards to search for wild pets
};

} // namespace Playerbot
