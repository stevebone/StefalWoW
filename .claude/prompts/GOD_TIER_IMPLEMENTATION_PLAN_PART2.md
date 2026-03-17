# GOD-TIER BOT SYSTEM - Implementierungsplan Teil 2

**Fortsetzung von Teil 1**

---

## TASK 4: GatheringSessionManager (12h)

### 4.1 Dateien

```
src/modules/Playerbot/Humanization/Sessions/
├── GatheringSessionManager.h
└── GatheringSessionManager.cpp
```

### 4.2 GatheringSessionManager.h

```cpp
#pragma once

#include "Define.h"
#include "ActivityType.h"
#include "Position.h"
#include "Duration.h"
#include <vector>
#include <memory>

class Player;

namespace Playerbot
{

class BotAI;
class HumanizationManager;

/**
 * @brief Gathering node types
 */
enum class GatheringType : uint8
{
    MINING,
    HERBALISM,
    SKINNING,
    ALL         // Any available
};

/**
 * @brief Gathering route definition
 */
struct GatheringRoute
{
    uint32 routeId;
    std::string name;
    uint32 zoneId;
    GatheringType type;
    uint16 minSkill;
    uint16 maxSkill;
    std::vector<Position> waypoints;
    float estimatedNodesPerHour;
};

/**
 * @brief Current gathering session state
 */
struct GatheringSessionState
{
    GatheringType type = GatheringType::ALL;
    uint32 zoneId = 0;
    GatheringRoute const* activeRoute = nullptr;
    
    // Progress
    Milliseconds sessionDuration{0};
    Milliseconds elapsedTime{0};
    uint32 nodesGathered = 0;
    uint32 itemsLooted = 0;
    uint32 skillPointsGained = 0;
    
    // Route tracking
    size_t currentWaypointIndex = 0;
    uint32 routeCompletions = 0;
    
    // Humanization
    bool isPaused = false;
    Milliseconds timeSinceLastBreak{0};
    uint32 breaksTaken = 0;
};

/**
 * @brief Manages dedicated gathering sessions (30-60+ minutes)
 * 
 * Unlike opportunistic gathering, this creates focused farming sessions
 * where the bot:
 * - Commits to gathering for 30-60+ minutes
 * - Stays in one zone/route
 * - Takes natural breaks
 * - Continues until session ends or bags full
 */
class TC_GAME_API GatheringSessionManager
{
public:
    explicit GatheringSessionManager(Player* bot, BotAI* ai, HumanizationManager* humanization);
    ~GatheringSessionManager();
    
    // Non-copyable
    GatheringSessionManager(GatheringSessionManager const&) = delete;
    GatheringSessionManager& operator=(GatheringSessionManager const&) = delete;
    
    // ========================================================================
    // LIFECYCLE
    // ========================================================================
    
    void Initialize();
    void Update(uint32 diff);
    
    // ========================================================================
    // SESSION MANAGEMENT
    // ========================================================================
    
    /**
     * @brief Start a gathering session
     * @param type What to gather (or ALL for any)
     * @param duration Target duration (0 = config default: 30-60 min)
     * @param zoneId Specific zone (0 = auto-select best)
     * @return true if session started
     */
    bool StartSession(GatheringType type = GatheringType::ALL,
                      Milliseconds duration = Milliseconds(0),
                      uint32 zoneId = 0);
    
    /**
     * @brief End current gathering session
     */
    void EndSession();
    
    /**
     * @brief Check if in gathering session
     */
    bool IsInSession() const { return _state != nullptr; }
    
    /**
     * @brief Get current session state
     */
    GatheringSessionState const* GetState() const { return _state.get(); }
    
    /**
     * @brief Get remaining session time
     */
    Milliseconds GetRemainingTime() const;
    
    /**
     * @brief Get session progress (0.0 - 1.0)
     */
    float GetProgress() const;
    
    // ========================================================================
    // ROUTE MANAGEMENT
    // ========================================================================
    
    /**
     * @brief Get available routes for gathering type
     */
    std::vector<GatheringRoute const*> GetAvailableRoutes(GatheringType type) const;
    
    /**
     * @brief Get best route for bot's level/skill
     */
    GatheringRoute const* GetBestRoute(GatheringType type) const;
    
    /**
     * @brief Select specific route
     */
    void SetRoute(uint32 routeId);
    
    // ========================================================================
    // HUMANIZATION
    // ========================================================================
    
    /**
     * @brief Check if should take a break
     */
    bool ShouldTakeBreak() const;
    
    /**
     * @brief Take a short break
     */
    void TakeBreak(Milliseconds duration);
    
    /**
     * @brief Check if on break
     */
    bool IsOnBreak() const;
    
    // ========================================================================
    // CALLBACKS
    // ========================================================================
    
    /**
     * @brief Called when node is gathered
     */
    void OnNodeGathered(uint32 nodeEntry, uint32 itemCount);
    
    /**
     * @brief Called when skill point gained
     */
    void OnSkillUp(uint32 skillId, uint16 newValue);
    
    /**
     * @brief Called when bags are full
     */
    void OnBagsFull();
    
private:
    // Session logic
    void UpdateSession(uint32 diff);
    void UpdateRoute(uint32 diff);
    void UpdateBreak(uint32 diff);
    
    // Route selection
    GatheringRoute const* SelectRoute(GatheringType type, uint32 zoneId);
    bool IsRouteValid(GatheringRoute const* route) const;
    
    // Navigation
    void NavigateToNextWaypoint();
    bool IsAtWaypoint() const;
    void AdvanceWaypoint();
    
    // Humanization
    void SimulateHumanBehavior(uint32 diff);
    Milliseconds CalculateBreakDuration() const;
    
    // Data loading
    void LoadRoutes();
    
    Player* _bot;
    BotAI* _ai;
    HumanizationManager* _humanization;
    
    // Session state
    std::unique_ptr<GatheringSessionState> _state;
    
    // Break state
    bool _onBreak = false;
    Milliseconds _breakRemaining{0};
    
    // Route database
    static std::vector<GatheringRoute> _routes;
    static bool _routesLoaded;
    
    // Constants
    static constexpr Milliseconds MIN_SESSION_DURATION{1800000};  // 30 min
    static constexpr Milliseconds MAX_SESSION_DURATION{3600000};  // 60 min
    static constexpr Milliseconds BREAK_INTERVAL{600000};         // 10 min between breaks
    static constexpr Milliseconds MIN_BREAK{30000};               // 30 sec
    static constexpr Milliseconds MAX_BREAK{120000};              // 2 min
};

} // namespace Playerbot
```

---

## TASK 5: CityLifeBehaviorManager (10h)

### 5.1 CityLifeBehaviorManager.h

```cpp
#pragma once

#include "Define.h"
#include "Position.h"
#include "Duration.h"
#include <vector>
#include <memory>

class Player;
class Creature;
class GameObject;

namespace Playerbot
{

class BotAI;
class HumanizationManager;

/**
 * @brief City activities for human-like behavior
 */
enum class CityActivity : uint8
{
    NONE = 0,
    
    // Economic
    AH_BROWSING,            // Standing at AH, checking prices
    AH_POSTING,             // Posting auctions
    AH_COLLECTING,          // Collecting gold/items
    MAILBOX_CHECK,          // Checking mail
    BANK_VISIT,             // Visiting bank
    GUILD_BANK,             // Guild bank
    VENDOR_BROWSE,          // Looking at vendors
    REPAIR,                 // Repairing gear
    
    // Social
    INN_REST,               // Sitting in inn (rested XP)
    WANDERING,              // Walking around town
    EMOTING,                // Using emotes
    WATCHING,               // Standing and watching
    
    // Services
    CLASS_TRAINER,          // Class trainer
    PROFESSION_TRAINER,     // Profession trainer
    RIDING_TRAINER,         // Riding trainer
    TRANSMOG,               // Transmogrifier
    BARBER,                 // Barbershop
    
    MAX_ACTIVITY
};

/**
 * @brief City location cache
 */
struct CityLocations
{
    uint32 cityId;
    std::string name;
    Position auctionHouse;
    Position mailbox;
    Position bank;
    Position inn;
    Position classTrainer;
    std::vector<std::pair<uint32, Position>> professionTrainers;  // skillId -> position
    Position ridingTrainer;
    Position transmogrifier;
    Position barber;
    Position repairVendor;
    std::vector<Position> wanderPoints;  // Points for idle wandering
};

/**
 * @brief Current city session state
 */
struct CitySessionState
{
    CityActivity currentActivity = CityActivity::NONE;
    Milliseconds activityDuration{0};
    Milliseconds activityElapsed{0};
    
    // Session totals
    Milliseconds totalCityTime{0};
    Milliseconds targetCityTime{0};
    
    // Activity tracking
    std::vector<CityActivity> completedActivities;
    uint32 activitiesCompleted = 0;
    
    // Location
    CityLocations const* city = nullptr;
    Position currentTarget;
};

/**
 * @brief Manages city life behaviors for human-like gameplay
 * 
 * When entering a city, bots will:
 * - Browse AH for 10-30 minutes
 * - Check mail
 * - Visit inn for rested XP
 * - Wander around
 * - Eventually leave
 */
class TC_GAME_API CityLifeBehaviorManager
{
public:
    explicit CityLifeBehaviorManager(Player* bot, BotAI* ai, HumanizationManager* humanization);
    ~CityLifeBehaviorManager();
    
    // ========================================================================
    // LIFECYCLE
    // ========================================================================
    
    void Initialize();
    void Update(uint32 diff);
    
    // ========================================================================
    // SESSION MANAGEMENT
    // ========================================================================
    
    /**
     * @brief Start city life session
     * @param duration Target time in city (0 = config default)
     * @return true if in a city and session started
     */
    bool StartCityLife(Milliseconds duration = Milliseconds(0));
    
    /**
     * @brief End city life session
     */
    void EndCityLife();
    
    /**
     * @brief Check if in city life session
     */
    bool IsInCityLife() const { return _state != nullptr; }
    
    /**
     * @brief Get current activity
     */
    CityActivity GetCurrentActivity() const;
    
    /**
     * @brief Get remaining city time
     */
    Milliseconds GetRemainingTime() const;
    
    // ========================================================================
    // SPECIFIC ACTIVITIES
    // ========================================================================
    
    /**
     * @brief Start AH browsing
     */
    void StartAHBrowsing();
    
    /**
     * @brief Start AH posting (sell items)
     */
    void StartAHPosting();
    
    /**
     * @brief Check mailbox
     */
    void CheckMailbox();
    
    /**
     * @brief Visit bank
     */
    void VisitBank();
    
    /**
     * @brief Rest in inn
     */
    void RestInInn();
    
    /**
     * @brief Visit class trainer
     */
    void VisitClassTrainer();
    
    /**
     * @brief Visit profession trainer
     */
    void VisitProfessionTrainer(uint32 skillId);
    
    /**
     * @brief Repair gear
     */
    void RepairGear();
    
    /**
     * @brief Just wander around
     */
    void StartWandering();
    
    // ========================================================================
    // CITY DETECTION
    // ========================================================================
    
    /**
     * @brief Check if bot is in a city
     */
    bool IsInCity() const;
    
    /**
     * @brief Get current city info
     */
    CityLocations const* GetCurrentCity() const;
    
    /**
     * @brief Check if city has specific service
     */
    bool CityHasService(CityActivity activity) const;
    
private:
    // Activity management
    void UpdateActivity(uint32 diff);
    void SelectNextActivity();
    void StartActivity(CityActivity activity);
    void CompleteActivity();
    
    // Navigation
    void NavigateToActivityLocation(CityActivity activity);
    bool IsAtActivityLocation() const;
    
    // Activity implementations
    void UpdateAHBrowsing(uint32 diff);
    void UpdateMailbox(uint32 diff);
    void UpdateInnRest(uint32 diff);
    void UpdateWandering(uint32 diff);
    void UpdateTrainerVisit(uint32 diff);
    
    // Humanization
    void PerformIdleBehavior();
    void DoRandomEmote();
    
    // Data loading
    void LoadCityData();
    CityLocations const* DetectCurrentCity() const;
    
    Player* _bot;
    BotAI* _ai;
    HumanizationManager* _humanization;
    
    // Session state
    std::unique_ptr<CitySessionState> _state;
    
    // City database
    static std::vector<CityLocations> _cities;
    static bool _citiesLoaded;
    
    // Constants
    static constexpr Milliseconds MIN_CITY_TIME{600000};      // 10 min
    static constexpr Milliseconds MAX_CITY_TIME{1800000};     // 30 min
    static constexpr Milliseconds AH_BROWSE_TIME{900000};     // 15 min
    static constexpr Milliseconds MAILBOX_TIME{120000};       // 2 min
    static constexpr Milliseconds INN_REST_TIME{600000};      // 10 min
    static constexpr Milliseconds TRAINER_TIME{60000};        // 1 min
};

} // namespace Playerbot
```

---

## TASK 6: AchievementManager (20h)

### 6.1 Dateien

```
src/modules/Playerbot/Humanization/Achievements/
├── AchievementManager.h
├── AchievementManager.cpp
├── AchievementDatabase.h
├── AchievementDatabase.cpp
├── AchievementGrinder.h
└── AchievementGrinder.cpp
```

### 6.2 AchievementManager.h

```cpp
#pragma once

#include "Define.h"
#include "Duration.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>

class Player;

namespace Playerbot
{

class BotAI;

/**
 * @brief Achievement categories
 */
enum class AchievementCategory : uint8
{
    GENERAL,
    QUESTS,
    EXPLORATION,
    PVP,
    DUNGEONS_RAIDS,
    PROFESSIONS,
    REPUTATION,
    WORLD_EVENTS,
    PET_BATTLES,
    COLLECTIONS,
    EXPANSION_FEATURES,
    FEATS_OF_STRENGTH,
    LEGACY
};

/**
 * @brief Achievement difficulty for prioritization
 */
enum class AchievementDifficulty : uint8
{
    TRIVIAL,        // Auto-complete or very easy
    EASY,           // Simple tasks
    MEDIUM,         // Requires some effort
    HARD,           // Significant effort
    VERY_HARD,      // Major time investment
    EXTREME         // Extremely difficult/rare
};

/**
 * @brief Achievement progress info
 */
struct AchievementProgressInfo
{
    uint32 achievementId;
    std::string name;
    std::string description;
    AchievementCategory category;
    AchievementDifficulty difficulty;
    
    // Progress
    bool isComplete;
    float progress;             // 0.0 - 1.0
    
    // Criteria breakdown
    struct CriteriaInfo
    {
        uint32 criteriaId;
        std::string description;
        uint32 current;
        uint32 required;
        bool isComplete;
    };
    std::vector<CriteriaInfo> criteria;
    
    // Rewards
    uint32 points;
    uint32 rewardTitle;
    uint32 rewardMount;
    uint32 rewardPet;
    uint32 rewardToy;
    uint32 rewardItem;
    
    // Estimation
    Milliseconds estimatedTimeToComplete;
};

/**
 * @brief Manages achievements for bots
 */
class TC_GAME_API AchievementManager
{
public:
    explicit AchievementManager(Player* bot, BotAI* ai);
    ~AchievementManager();
    
    // ========================================================================
    // LIFECYCLE
    // ========================================================================
    
    void Initialize();
    void Update(uint32 diff);
    
    // ========================================================================
    // ACHIEVEMENT TRACKING
    // ========================================================================
    
    /**
     * @brief Get total achievement points
     */
    uint32 GetTotalPoints() const;
    
    /**
     * @brief Get number of completed achievements
     */
    uint32 GetCompletedCount() const;
    
    /**
     * @brief Check if achievement is complete
     */
    bool IsComplete(uint32 achievementId) const;
    
    /**
     * @brief Get achievement progress
     */
    AchievementProgressInfo GetProgress(uint32 achievementId) const;
    
    /**
     * @brief Get all achievements with progress
     */
    std::vector<AchievementProgressInfo> GetAllAchievements() const;
    
    /**
     * @brief Get incomplete achievements
     */
    std::vector<AchievementProgressInfo> GetIncomplete() const;
    
    /**
     * @brief Get achievements by category
     */
    std::vector<AchievementProgressInfo> GetByCategory(AchievementCategory category) const;
    
    /**
     * @brief Get achievements close to completion (>80%)
     */
    std::vector<AchievementProgressInfo> GetNearlyComplete(float threshold = 0.8f) const;
    
    // ========================================================================
    // ACHIEVEMENT GRINDING
    // ========================================================================
    
    /**
     * @brief Start grinding specific achievement
     */
    bool StartGrinding(uint32 achievementId);
    
    /**
     * @brief Stop current grind
     */
    void StopGrinding();
    
    /**
     * @brief Check if currently grinding
     */
    bool IsGrinding() const { return _currentGrindTarget != 0; }
    
    /**
     * @brief Get current grind target
     */
    uint32 GetCurrentGrindTarget() const { return _currentGrindTarget; }
    
    /**
     * @brief Get recommended achievements to grind
     * Prioritized by: ease, rewards, progress
     */
    std::vector<uint32> GetRecommended(uint32 count = 10) const;
    
    /**
     * @brief Auto-select and grind best achievement
     */
    void AutoGrind();
    
    // ========================================================================
    // CATEGORY-SPECIFIC GRINDING
    // ========================================================================
    
    /**
     * @brief Grind exploration achievements
     */
    void GrindExploration();
    
    /**
     * @brief Grind quest achievements
     */
    void GrindQuests();
    
    /**
     * @brief Grind reputation achievements
     */
    void GrindReputation();
    
    /**
     * @brief Grind dungeon achievements
     */
    void GrindDungeons();
    
    /**
     * @brief Grind collection achievements
     */
    void GrindCollections();
    
    /**
     * @brief Grind PvP achievements
     */
    void GrindPvP();
    
    // ========================================================================
    // META ACHIEVEMENTS
    // ========================================================================
    
    /**
     * @brief Get meta achievement progress
     */
    struct MetaProgress
    {
        uint32 achievementId;
        std::string name;
        std::vector<uint32> requiredAchievements;
        std::vector<bool> completedParts;
        float overallProgress;
    };
    MetaProgress GetMetaProgress(uint32 metaId) const;
    
    /**
     * @brief Start grinding meta achievement
     */
    void GrindMeta(uint32 metaId);
    
private:
    // Progress calculation
    float CalculateProgress(uint32 achievementId) const;
    AchievementDifficulty EstimateDifficulty(uint32 achievementId) const;
    Milliseconds EstimateTime(uint32 achievementId) const;
    
    // Grinding logic
    void UpdateGrinding(uint32 diff);
    void ExecuteGrindAction();
    
    // Category grinding
    void GrindExplorationStep();
    void GrindQuestStep();
    void GrindReputationStep();
    
    Player* _bot;
    BotAI* _ai;
    
    // Grinding state
    uint32 _currentGrindTarget = 0;
    AchievementCategory _currentCategory = AchievementCategory::GENERAL;
    
    // Cache
    mutable std::unordered_map<uint32, AchievementProgressInfo> _progressCache;
    mutable uint32 _lastCacheUpdate = 0;
    
    // Constants
    static constexpr uint32 CACHE_DURATION = 60000;  // 1 minute
};

} // namespace Playerbot
```

---

## TASKS 7-14: Übersicht

### TASK 7: Mount Collection System (15h)

```cpp
// MountCollectionManager.h
class TC_GAME_API MountCollectionManager
{
public:
    // Get all obtainable mounts
    std::vector<MountInfo> GetObtainableMounts() const;
    
    // Start farming specific mount
    void FarmMount(uint32 mountSpellId);
    
    // Get mounts by source
    std::vector<MountInfo> GetMountsBySource(MountSource source) const;
    
    // Farm old raid mounts
    void FarmRaidMounts();
    
    // Farm reputation mounts
    void FarmRepMounts();
    
    // Farm achievement mounts
    void FarmAchievementMounts();
};
```

### TASK 8: Pet Collection System (15h)

```cpp
// PetCollectionManager.h
class TC_GAME_API PetCollectionManager
{
public:
    // Get all collectible pets
    std::vector<PetInfo> GetCollectiblePets() const;
    
    // Start pet collection
    void StartCollection();
    
    // Farm specific pet
    void FarmPet(uint32 speciesId);
    
    // Capture wild pets
    void CaptureWildPets(uint32 zoneId);
    
    // Buy vendor pets
    void BuyVendorPets();
};
```

### TASK 9: ReputationGrindManager (15h)

```cpp
// ReputationGrindManager.h
class TC_GAME_API ReputationGrindManager
{
public:
    // Get grindable factions
    std::vector<FactionInfo> GetGrindableFactions() const;
    
    // Start reputation grind
    void StartGrind(uint32 factionId, Standing target = Standing::EXALTED);
    
    // Get best grind method
    RepMethod GetBestMethod(uint32 factionId) const;
    
    // Get time to target standing
    Milliseconds GetTimeToStanding(uint32 factionId, Standing target) const;
    
    // Get recommended factions
    std::vector<uint32> GetRecommended() const;
};
```

### TASK 10: Achievement Grinding (15h)

Integration mit AchievementManager - spezifische Grind-Logik:

```cpp
// AchievementGrinder.h
class TC_GAME_API AchievementGrinder
{
public:
    // Execute exploration achievement
    void ExecuteExplorationAchievement(uint32 achievementId);
    
    // Execute quest achievement
    void ExecuteQuestAchievement(uint32 achievementId);
    
    // Execute kill achievement
    void ExecuteKillAchievement(uint32 achievementId);
    
    // Execute dungeon achievement
    void ExecuteDungeonAchievement(uint32 achievementId);
    
    // Execute collection achievement
    void ExecuteCollectionAchievement(uint32 achievementId);
};
```

### TASK 11: FishingSessionManager (10h)

```cpp
// FishingSessionManager.h
class TC_GAME_API FishingSessionManager
{
public:
    // Start fishing session (30-60 min)
    bool StartSession(Milliseconds duration = Milliseconds(0));
    
    // End session
    void EndSession();
    
    // Find fishing spot
    Position FindFishingSpot() const;
    
    // Humanization
    bool ShouldSitDown() const;
    void DoFishingEmote();
    void LookAround();
};
```

### TASK 12: AFKSimulator (10h)

```cpp
// AFKSimulator.h
class TC_GAME_API AFKSimulator
{
public:
    // Check if should go AFK
    bool ShouldGoAFK() const;
    
    // Start AFK
    void StartAFK(Milliseconds duration = Milliseconds(0));
    
    // End AFK
    void EndAFK();
    
    // Check if AFK
    bool IsAFK() const;
    
    // Get remaining AFK time
    Milliseconds GetRemainingTime() const;
    
    // AFK behaviors
    void SitDown();
    void UseAFKEmote();
    void StandStill();
};
```

### TASK 13: GoldFarmingManager (20h)

```cpp
// GoldFarmingManager.h
class TC_GAME_API GoldFarmingManager
{
public:
    enum class Strategy
    {
        RAW_GOLD,           // Kill mobs
        GATHERING,          // Farm materials
        CRAFTING,           // Craft and sell
        AUCTION_FLIPPING,   // Buy low, sell high
        OLD_CONTENT,        // Solo old raids
        WORLD_QUESTS,       // WQ gold rewards
        MISSION_TABLE       // Missions
    };
    
    // Start gold farming
    void StartFarm(Strategy strategy);
    
    // Get recommended strategy
    Strategy GetRecommendedStrategy() const;
    
    // Get estimated GPH
    uint64 GetEstimatedGPH(Strategy strategy) const;
    
    // Set minimum gold target
    void SetMinimumGold(uint64 amount);
};
```

### TASK 14: Instance Automation (25h)

```cpp
// InstanceAutomationManager.h
class TC_GAME_API InstanceAutomationManager
{
public:
    enum class FarmMode
    {
        MOUNT_FARM,
        TRANSMOG_FARM,
        ACHIEVEMENT_FARM,
        GOLD_FARM,
        PET_FARM,
        COMPLETION
    };
    
    // Start instance farming
    void StartFarm(uint32 instanceId, FarmMode mode);
    
    // Get farmable instances
    std::vector<InstanceInfo> GetFarmableInstances(FarmMode mode) const;
    
    // Check lockouts
    std::vector<LockoutInfo> GetLockouts() const;
    
    // Solo old raid
    void SoloOldRaid(uint32 raidId);
    
    // Handle boss mechanics
    void HandleBossMechanic(uint32 bossId, uint32 mechanicId);
};
```

---

## ZUSAMMENFASSUNG ALLER DATEIEN

### Neue Verzeichnisstruktur

```
src/modules/Playerbot/Humanization/
├── CMakeLists.txt
├── Core/
│   ├── HumanizationManager.h/cpp
│   ├── HumanizationConfig.h/cpp
│   ├── PersonalityProfile.h/cpp
│   ├── ActivityType.h
│   └── HumanizationDefines.h
├── Sessions/
│   ├── ActivitySession.h
│   ├── ActivitySessionManager.h/cpp
│   ├── GatheringSessionManager.h/cpp
│   ├── FishingSessionManager.h/cpp
│   ├── SessionTransitions.h/cpp
│   └── QuestingSessionManager.h/cpp
├── CityLife/
│   ├── CityLifeBehaviorManager.h/cpp
│   ├── CityLocations.h/cpp
│   └── CityActivities.h
├── Simulation/
│   ├── AFKSimulator.h/cpp
│   ├── HumanErrorSimulator.h/cpp
│   └── DistractionSimulator.h/cpp
├── Mounts/
│   ├── RidingManager.h/cpp
│   ├── MountCollectionManager.h/cpp
│   └── MountDatabase.h/cpp
├── Achievements/
│   ├── AchievementManager.h/cpp
│   ├── AchievementGrinder.h/cpp
│   └── AchievementDatabase.h/cpp
├── Collections/
│   ├── PetCollectionManager.h/cpp
│   └── TransmogManager.h/cpp
├── Economy/
│   ├── GoldFarmingManager.h/cpp
│   └── AuctionStrategy.h/cpp
├── Reputation/
│   └── ReputationGrindManager.h/cpp
└── Instances/
    ├── InstanceAutomationManager.h/cpp
    └── BossMechanicsHandler.h/cpp

src/modules/Playerbot/Quest/
├── QuestCompletion.cpp      # MODIFY - Add new handlers
└── QuestCompletion.h        # MODIFY - Update enum
```

### Geschätzte Zeilen Code

| Komponente | Neue Zeilen | Modifizierte Zeilen |
|------------|-------------|---------------------|
| Quest Handlers (11 neue) | ~1,500 | ~200 |
| RidingManager | ~800 | - |
| HumanizationManager | ~1,200 | - |
| ActivitySessionManager | ~600 | - |
| GatheringSessionManager | ~500 | - |
| CityLifeBehaviorManager | ~700 | - |
| AchievementManager | ~1,000 | - |
| MountCollectionManager | ~500 | - |
| PetCollectionManager | ~500 | - |
| ReputationGrindManager | ~600 | - |
| FishingSessionManager | ~400 | - |
| AFKSimulator | ~300 | - |
| GoldFarmingManager | ~800 | - |
| InstanceAutomationManager | ~1,000 | - |
| **TOTAL** | **~10,400** | **~200** |

---

## ZEITPLAN

### Woche 1-2: P0 Core (80h)
- Task 1: Quest System (30h)
- Task 2: RidingManager (15h)
- Task 3: HumanizationManager (35h partial)

### Woche 3-4: P0 Completion (47h)
- Task 3: HumanizationManager (5h remaining)
- Task 4: GatheringSessionManager (12h)
- Task 5: CityLifeBehaviorManager (10h)
- Task 6: AchievementManager (20h)

### Woche 5-6: P1 Collections (45h)
- Task 7: MountCollectionManager (15h)
- Task 8: PetCollectionManager (15h)
- Task 10: Achievement Grinding (15h)

### Woche 7-8: P1 Economy & Sessions (45h)
- Task 9: ReputationGrindManager (15h)
- Task 11: FishingSessionManager (10h)
- Task 12: AFKSimulator (10h)
- Task 13 partial: GoldFarmingManager (10h)

### Woche 9-10: P1 Completion (35h)
- Task 13: GoldFarmingManager (10h remaining)
- Task 14: InstanceAutomationManager (25h)

**Total: ~252h über 10 Wochen**
