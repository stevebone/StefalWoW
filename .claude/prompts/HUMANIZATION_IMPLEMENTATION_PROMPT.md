# Humanization System - Implementation Prompt

**Date**: 2026-01-27  
**Priority**: P0 - CRITICAL for natural bot behavior  
**Estimated Effort**: 200h total (phased)

---

## 🎯 Ziel

Bots sollen sich wie **echte Spieler** verhalten:
- ✅ Gathering-Sessions von 30-60+ Minuten
- ✅ Idle-Zeit in Hauptstädten (AH, Mailbox, Inn)
- ✅ Cooking nach dem Sammeln
- ✅ Fishing als "Hobby" (30+ Min an einem Spot)
- ✅ Natürliche Pausen (AFK-Simulation)
- ✅ Fehler machen (suboptimale Routen)

---

## 📁 Neue Verzeichnisstruktur

```
src/modules/Playerbot/Humanization/
├── CMakeLists.txt
├── Core/
│   ├── HumanizationManager.h
│   ├── HumanizationManager.cpp
│   ├── HumanizationConfig.h
│   ├── HumanizationConfig.cpp
│   ├── PersonalityProfile.h
│   └── PersonalityProfile.cpp
│
├── Sessions/
│   ├── ActivitySession.h              # Base session struct
│   ├── ActivitySessionManager.h
│   ├── ActivitySessionManager.cpp
│   ├── GatheringSessionManager.h
│   ├── GatheringSessionManager.cpp
│   ├── FishingSessionManager.h
│   ├── FishingSessionManager.cpp
│   ├── QuestingSessionManager.h
│   └── QuestingSessionManager.cpp
│
├── CityLife/
│   ├── CityLifeBehaviorManager.h
│   ├── CityLifeBehaviorManager.cpp
│   ├── CityActivity.h                 # Enum + structs
│   ├── AuctionHouseBehavior.h
│   ├── AuctionHouseBehavior.cpp
│   ├── InnBehavior.h
│   ├── InnBehavior.cpp
│   ├── MailboxBehavior.h
│   └── MailboxBehavior.cpp
│
├── Simulation/
│   ├── AFKSimulator.h
│   ├── AFKSimulator.cpp
│   ├── HumanErrorSimulator.h
│   └── HumanErrorSimulator.cpp
│
└── Scheduling/
    ├── ActivityScheduler.h
    └── ActivityScheduler.cpp
```

---

## Phase 1: Core Framework (40h)

### Task 1.1: HumanizationManager (8h)

**File**: `Humanization/Core/HumanizationManager.h`

```cpp
#pragma once

#include "Define.h"
#include "PersonalityProfile.h"
#include <memory>

class Player;

namespace Playerbot
{

class BotAI;
class ActivitySessionManager;
class CityLifeBehaviorManager;
class AFKSimulator;

/**
 * @brief Central coordinator for human-like bot behavior
 * 
 * Manages:
 * - Activity sessions (gathering, questing, fishing)
 * - City life behaviors (AH, mailbox, inn)
 * - AFK simulation
 * - Personality-based decision modifications
 */
class TC_GAME_API HumanizationManager
{
public:
    explicit HumanizationManager(Player* bot, BotAI* ai);
    ~HumanizationManager();
    
    // Non-copyable
    HumanizationManager(const HumanizationManager&) = delete;
    HumanizationManager& operator=(const HumanizationManager&) = delete;
    
    // ========================================================================
    // LIFECYCLE
    // ========================================================================
    
    void Initialize();
    void Update(uint32 diff);
    void Shutdown();
    
    // ========================================================================
    // SESSION MANAGEMENT
    // ========================================================================
    
    /**
     * @brief Start a new activity session
     * @param type Activity type (GATHERING, QUESTING, FISHING, etc.)
     * @param minDuration Minimum session duration (ms)
     * @param maxDuration Maximum session duration (ms)
     * @return true if session started
     */
    bool StartSession(ActivityType type, 
                      Milliseconds minDuration = Milliseconds(1800000),  // 30 min
                      Milliseconds maxDuration = Milliseconds(5400000)); // 90 min
    
    /**
     * @brief End current session and transition to next activity
     */
    void EndCurrentSession();
    
    /**
     * @brief Check if in active session
     */
    bool IsInSession() const;
    
    /**
     * @brief Get remaining session time
     */
    Milliseconds GetRemainingSessionTime() const;
    
    /**
     * @brief Should we switch activity? (session ended, boredom, etc.)
     */
    bool ShouldSwitchActivity() const;
    
    // ========================================================================
    // CITY LIFE
    // ========================================================================
    
    /**
     * @brief Start city life behaviors
     * Called when entering a city/rest area
     */
    void StartCityLife();
    
    /**
     * @brief Check if in city life mode
     */
    bool IsInCityLife() const;
    
    /**
     * @brief Get current city activity
     */
    CityActivity GetCurrentCityActivity() const;
    
    // ========================================================================
    // AFK SIMULATION
    // ========================================================================
    
    /**
     * @brief Check if should go AFK
     */
    bool ShouldGoAFK() const;
    
    /**
     * @brief Start AFK behavior
     */
    void StartAFK();
    
    /**
     * @brief Check if currently AFK
     */
    bool IsAFK() const;
    
    // ========================================================================
    // PERSONALITY
    // ========================================================================
    
    /**
     * @brief Get personality profile
     */
    PersonalityProfile const& GetPersonality() const { return _personality; }
    
    /**
     * @brief Set personality profile
     */
    void SetPersonality(PersonalityProfile const& profile) { _personality = profile; }
    
    /**
     * @brief Generate random personality for this bot
     */
    void GenerateRandomPersonality();
    
    // ========================================================================
    // DECISION MODIFIERS
    // ========================================================================
    
    /**
     * @brief Modify action based on personality and humanization
     * Called by AI system before executing actions
     * 
     * @param actionId Original action
     * @return Modified action (or same if no change)
     */
    uint32 ModifyAction(uint32 actionId);
    
    /**
     * @brief Should make an error? (wrong path, forget quest, etc.)
     */
    bool ShouldMakeError(HumanErrorType errorType) const;
    
    // ========================================================================
    // MANAGERS ACCESS
    // ========================================================================
    
    ActivitySessionManager* GetSessionManager() { return _sessionManager.get(); }
    CityLifeBehaviorManager* GetCityLifeManager() { return _cityLifeManager.get(); }
    AFKSimulator* GetAFKSimulator() { return _afkSimulator.get(); }
    
private:
    Player* _bot;
    BotAI* _ai;
    
    // Sub-managers
    std::unique_ptr<ActivitySessionManager> _sessionManager;
    std::unique_ptr<CityLifeBehaviorManager> _cityLifeManager;
    std::unique_ptr<AFKSimulator> _afkSimulator;
    
    // Personality
    PersonalityProfile _personality;
    
    // State
    bool _initialized = false;
    uint32 _lastUpdateTime = 0;
};

} // namespace Playerbot
```

### Task 1.2: Activity Types & Session Struct (4h)

**File**: `Humanization/Sessions/ActivitySession.h`

```cpp
#pragma once

#include "Define.h"
#include "Duration.h"
#include "Position.h"

namespace Playerbot
{

/**
 * @brief Types of activities for session management
 */
enum class ActivityType : uint8
{
    NONE = 0,
    
    // Primary activities
    QUESTING,           // Quest completion
    GRINDING,           // Mob grinding for XP
    GATHERING,          // Mining, Herbalism, Skinning
    FISHING,            // Fishing (hobby mode)
    DUNGEON,            // Running dungeons
    PVP,                // Battlegrounds, arenas
    
    // Secondary activities
    CRAFTING,           // Profession crafting
    COOKING,            // Cooking specifically
    CITY_LIFE,          // AH, mailbox, vendors
    EXPLORING,          // Just wandering around
    SOCIALIZING,        // Interacting with others
    
    // Passive
    RESTING,            // In inn, gaining rested XP
    AFK,                // AFK simulation
    TRAVELING,          // Moving between zones
    
    MAX
};

/**
 * @brief Represents an activity session with duration and state
 */
struct ActivitySession
{
    ActivityType type = ActivityType::NONE;
    
    // Timing
    Milliseconds plannedDuration{0};      // How long we planned to do this
    Milliseconds elapsedTime{0};          // How long we've been doing it
    Milliseconds minDuration{1800000};    // 30 min minimum
    Milliseconds maxDuration{5400000};    // 90 min maximum
    
    // Location
    Position startPosition;               // Where session started
    uint32 startZoneId = 0;               // Zone where session started
    
    // State
    bool isActive = false;
    bool isPaused = false;
    uint32 pauseCount = 0;                // How many pauses taken
    Milliseconds totalPauseTime{0};       // Total time paused
    
    // Metrics
    uint32 objectivesCompleted = 0;       // Quests/nodes/fish/etc.
    uint32 itemsGained = 0;
    uint32 xpGained = 0;
    
    // Humanization
    float boredomLevel = 0.0f;            // 0.0 = engaged, 1.0 = bored
    float energyLevel = 1.0f;             // 1.0 = fresh, 0.0 = tired
    Milliseconds nextBreakAt{0};          // When to take a break
    
    // Methods
    bool IsExpired() const 
    { 
        return elapsedTime >= plannedDuration; 
    }
    
    float GetProgress() const 
    { 
        return plannedDuration.count() > 0 
            ? static_cast<float>(elapsedTime.count()) / plannedDuration.count() 
            : 0.0f;
    }
    
    Milliseconds GetRemainingTime() const 
    { 
        return plannedDuration > elapsedTime 
            ? plannedDuration - elapsedTime 
            : Milliseconds(0);
    }
};

/**
 * @brief Session transition rules
 */
struct SessionTransition
{
    ActivityType fromType;
    ActivityType toType;
    float probability;                    // Chance of this transition
    Milliseconds cooldown;                // Min time before this transition
    
    // Conditions
    bool requiresCity = false;            // Must be in city
    bool requiresSafeArea = false;        // Must be safe
    uint8 minLevel = 1;                   // Minimum level required
};

/**
 * @brief Session factory settings per activity type
 */
struct SessionTemplate
{
    ActivityType type;
    Milliseconds defaultMinDuration;
    Milliseconds defaultMaxDuration;
    float breakChance;                    // Chance of taking breaks
    Milliseconds breakInterval;           // How often to consider breaks
    Milliseconds breakMinDuration;
    Milliseconds breakMaxDuration;
    bool allowInterruption;               // Can be interrupted by events
    float boredomRate;                    // How fast boredom increases
};

} // namespace Playerbot
```

### Task 1.3: PersonalityProfile (8h)

**File**: `Humanization/Core/PersonalityProfile.h`

```cpp
#pragma once

#include "Define.h"
#include <array>
#include <string>

namespace Playerbot
{

/**
 * @brief Human error types for simulation
 */
enum class HumanErrorType : uint8
{
    WRONG_PATH,             // Takes inefficient route
    FORGOT_QUEST,           // Forgets to pick up/turn in
    FORGOT_VENDOR,          // Forgets to sell/repair
    SUBOPTIMAL_ROTATION,    // Uses wrong spell priority
    DISTRACTION,            // Stops for no reason
    OVERAGGRO,              // Pulls too many mobs
    MAX
};

/**
 * @brief Personality types affecting behavior
 */
enum class PersonalityType : uint8
{
    CASUAL,         // Relaxed, lots of breaks, city time
    EFFICIENT,      // Focused, minimal breaks, optimal play
    SOCIAL,         // Lots of emotes, stays in cities
    EXPLORER,       // Wanders, takes scenic routes
    FARMER,         // Long gathering sessions
    COLLECTOR,      // Focuses on achievements, items
    RANDOM          // Mix of all
};

/**
 * @brief Bot personality profile affecting humanization
 */
struct PersonalityProfile
{
    PersonalityType type = PersonalityType::RANDOM;
    
    // Activity preferences (0.0 - 1.0, higher = more likely)
    float questingPreference = 0.5f;
    float gatheringPreference = 0.3f;
    float fishingPreference = 0.2f;
    float cityLifePreference = 0.4f;
    float dungeonPreference = 0.3f;
    float pvpPreference = 0.2f;
    float explorationPreference = 0.3f;
    
    // Session duration modifiers (multiplier)
    float sessionDurationModifier = 1.0f;    // 1.0 = normal, 0.5 = short, 2.0 = long
    float breakFrequencyModifier = 1.0f;     // 1.0 = normal breaks
    
    // AFK tendencies
    float afkFrequency = 0.5f;               // How often to go AFK (0.0 - 1.0)
    float afkDurationModifier = 1.0f;        // How long AFKs last
    
    // Error tendencies (0.0 - 1.0)
    std::array<float, static_cast<size_t>(HumanErrorType::MAX)> errorChances = {
        0.05f,  // WRONG_PATH
        0.02f,  // FORGOT_QUEST
        0.03f,  // FORGOT_VENDOR
        0.05f,  // SUBOPTIMAL_ROTATION
        0.10f,  // DISTRACTION
        0.02f   // OVERAGGRO
    };
    
    // Social tendencies
    float emoteFrequency = 0.3f;             // How often to emote
    float chatResponsiveness = 0.5f;         // How likely to respond to chat
    
    // Time-of-day preferences
    std::array<float, 24> hourlyActivityLevel;  // Activity level per hour
    
    // City life preferences
    float ahBrowsingDuration = 1.0f;         // Multiplier for AH time
    float innRestChance = 0.3f;              // Chance to rest in inn
    float mailCheckFrequency = 0.5f;         // How often to check mail
    
    // Generate random profile
    static PersonalityProfile GenerateRandom();
    
    // Generate profile for specific type
    static PersonalityProfile Generate(PersonalityType type);
    
    // Get error chance for specific type
    float GetErrorChance(HumanErrorType errorType) const
    {
        return errorChances[static_cast<size_t>(errorType)];
    }
    
    // Get activity level for current hour
    float GetCurrentActivityLevel() const;
    
    // Serialize/deserialize for persistence
    std::string Serialize() const;
    static PersonalityProfile Deserialize(std::string const& data);
};

} // namespace Playerbot
```

### Task 1.4: ActivitySessionManager (12h)

**File**: `Humanization/Sessions/ActivitySessionManager.h`

```cpp
#pragma once

#include "Define.h"
#include "ActivitySession.h"
#include "PersonalityProfile.h"
#include <memory>
#include <vector>
#include <functional>

class Player;

namespace Playerbot
{

class BotAI;
class HumanizationManager;

/**
 * @brief Manages activity sessions ensuring human-like duration and transitions
 * 
 * Key features:
 * - Enforces minimum session durations (30+ minutes for gathering)
 * - Manages session transitions (quest → city → gather → fish)
 * - Handles breaks within sessions
 * - Tracks boredom and energy levels
 */
class TC_GAME_API ActivitySessionManager
{
public:
    ActivitySessionManager(Player* bot, BotAI* ai, HumanizationManager* humanization);
    ~ActivitySessionManager();
    
    // ========================================================================
    // LIFECYCLE
    // ========================================================================
    
    void Initialize();
    void Update(uint32 diff);
    
    // ========================================================================
    // SESSION CONTROL
    // ========================================================================
    
    /**
     * @brief Start a new activity session
     * 
     * @param type Activity type
     * @param minDuration Minimum duration (default from template)
     * @param maxDuration Maximum duration (default from template)
     * @return true if session started successfully
     */
    bool StartSession(ActivityType type,
                      Milliseconds minDuration = Milliseconds(0),
                      Milliseconds maxDuration = Milliseconds(0));
    
    /**
     * @brief End current session
     * @param forceEnd If true, ends even if minimum duration not reached
     */
    void EndSession(bool forceEnd = false);
    
    /**
     * @brief Pause current session (e.g., for combat)
     */
    void PauseSession();
    
    /**
     * @brief Resume paused session
     */
    void ResumeSession();
    
    /**
     * @brief Take a break within the session
     * @param duration Break duration
     */
    void TakeBreak(Milliseconds duration);
    
    // ========================================================================
    // SESSION QUERIES
    // ========================================================================
    
    /**
     * @brief Get current session (nullptr if none)
     */
    ActivitySession const* GetCurrentSession() const { return _currentSession.get(); }
    
    /**
     * @brief Check if session is active
     */
    bool HasActiveSession() const { return _currentSession && _currentSession->isActive; }
    
    /**
     * @brief Get current activity type
     */
    ActivityType GetCurrentActivityType() const;
    
    /**
     * @brief Check if can end session (min duration reached)
     */
    bool CanEndSession() const;
    
    /**
     * @brief Check if should end session (max duration, boredom, etc.)
     */
    bool ShouldEndSession() const;
    
    /**
     * @brief Get remaining session time
     */
    Milliseconds GetRemainingTime() const;
    
    /**
     * @brief Get session progress (0.0 - 1.0)
     */
    float GetSessionProgress() const;
    
    // ========================================================================
    // TRANSITION MANAGEMENT
    // ========================================================================
    
    /**
     * @brief Get recommended next activity based on current state
     */
    ActivityType GetRecommendedNextActivity() const;
    
    /**
     * @brief Register custom transition callback
     */
    using TransitionCallback = std::function<void(ActivityType from, ActivityType to)>;
    void OnSessionTransition(TransitionCallback callback);
    
    // ========================================================================
    // METRICS
    // ========================================================================
    
    /**
     * @brief Record objective completion (quest, node gathered, etc.)
     */
    void RecordObjectiveComplete();
    
    /**
     * @brief Record item gained
     */
    void RecordItemGained(uint32 count = 1);
    
    /**
     * @brief Record XP gained
     */
    void RecordXPGained(uint32 xp);
    
    // ========================================================================
    // CONFIGURATION
    // ========================================================================
    
    /**
     * @brief Set session template for activity type
     */
    void SetSessionTemplate(ActivityType type, SessionTemplate const& tmpl);
    
    /**
     * @brief Get session template
     */
    SessionTemplate GetSessionTemplate(ActivityType type) const;
    
private:
    // Initialization
    void InitializeSessionTemplates();
    void InitializeTransitionRules();
    
    // Session logic
    void UpdateBoredom(uint32 diff);
    void UpdateEnergy(uint32 diff);
    void CheckForBreak();
    ActivityType SelectNextActivity() const;
    Milliseconds CalculateSessionDuration(ActivityType type) const;
    
    // Members
    Player* _bot;
    BotAI* _ai;
    HumanizationManager* _humanization;
    
    std::unique_ptr<ActivitySession> _currentSession;
    std::vector<SessionTemplate> _sessionTemplates;
    std::vector<SessionTransition> _transitionRules;
    std::vector<TransitionCallback> _transitionCallbacks;
    
    // Break tracking
    bool _onBreak = false;
    Milliseconds _breakTimeRemaining{0};
    
    // History for variety
    std::vector<ActivityType> _recentActivities;
    static constexpr size_t MAX_ACTIVITY_HISTORY = 10;
};

} // namespace Playerbot
```

### Task 1.5: HumanizationConfig (4h)

**File**: `Humanization/Core/HumanizationConfig.h`

```cpp
#pragma once

#include "Define.h"
#include "Duration.h"

namespace Playerbot
{

/**
 * @brief Configuration for humanization system
 * Loaded from worldserver.conf
 */
struct HumanizationConfig
{
    // ========================================================================
    // GLOBAL SETTINGS
    // ========================================================================
    
    bool enabled = true;
    
    // ========================================================================
    // SESSION SETTINGS
    // ========================================================================
    
    // Default session durations
    Milliseconds defaultMinSessionDuration{1800000};    // 30 minutes
    Milliseconds defaultMaxSessionDuration{5400000};    // 90 minutes
    
    // Gathering sessions (longer)
    Milliseconds gatheringMinDuration{1800000};         // 30 minutes minimum!
    Milliseconds gatheringMaxDuration{3600000};         // 60 minutes max
    
    // Fishing sessions
    Milliseconds fishingMinDuration{1800000};           // 30 minutes
    Milliseconds fishingMaxDuration{3600000};           // 60 minutes
    
    // Questing sessions
    Milliseconds questingMinDuration{1800000};          // 30 minutes
    Milliseconds questingMaxDuration{5400000};          // 90 minutes
    
    // City life
    Milliseconds cityLifeMinDuration{600000};           // 10 minutes
    Milliseconds cityLifeMaxDuration{1800000};          // 30 minutes
    
    // ========================================================================
    // BREAK SETTINGS
    // ========================================================================
    
    float breakChance = 0.1f;                           // 10% chance per check
    Milliseconds breakCheckInterval{300000};            // Check every 5 minutes
    Milliseconds breakMinDuration{60000};               // 1 minute min
    Milliseconds breakMaxDuration{300000};              // 5 minutes max
    
    // ========================================================================
    // AFK SETTINGS
    // ========================================================================
    
    bool afkEnabled = true;
    Milliseconds afkMinInterval{1800000};               // Min 30 min between AFKs
    Milliseconds afkMaxInterval{3600000};               // Max 60 min between AFKs
    Milliseconds afkMinDuration{60000};                 // Min 1 min AFK
    Milliseconds afkMaxDuration{300000};                // Max 5 min AFK
    float afkChance = 0.3f;                             // 30% base chance
    
    // ========================================================================
    // CITY LIFE SETTINGS
    // ========================================================================
    
    bool cityLifeEnabled = true;
    Milliseconds ahBrowsingMinDuration{300000};         // 5 minutes
    Milliseconds ahBrowsingMaxDuration{1200000};        // 20 minutes
    Milliseconds mailboxDuration{120000};               // 2 minutes
    Milliseconds innRestMinDuration{300000};            // 5 minutes
    Milliseconds innRestMaxDuration{900000};            // 15 minutes
    float innRestChance = 0.3f;                         // 30% chance to rest in inn
    
    // ========================================================================
    // ERROR SIMULATION
    // ========================================================================
    
    bool errorSimulationEnabled = true;
    float wrongPathChance = 0.05f;                      // 5%
    float forgotQuestChance = 0.02f;                    // 2%
    float forgotVendorChance = 0.03f;                   // 3%
    float suboptimalRotationChance = 0.05f;             // 5%
    float distractionChance = 0.10f;                    // 10%
    
    // ========================================================================
    // DAY/NIGHT SETTINGS
    // ========================================================================
    
    bool dayNightEnabled = true;
    float nightActivityReduction = 0.5f;                // 50% less active at night
    uint8 nightStartHour = 23;
    uint8 nightEndHour = 7;
    bool logoutAtNight = false;                         // If true, log out at night
    bool sleepInInn = true;                             // If true, go to inn at night
    
    // ========================================================================
    // METHODS
    // ========================================================================
    
    static HumanizationConfig LoadFromConfig();
    void SaveToConfig() const;
    
    static HumanizationConfig& Instance()
    {
        static HumanizationConfig instance;
        return instance;
    }
};

#define sHumanizationConfig HumanizationConfig::Instance()

} // namespace Playerbot
```

---

## Phase 2: Session Managers (50h)

### Task 2.1: GatheringSessionManager (12h)

**Core Feature**: Gathering für 30-60+ Minuten am Stück

**File**: `Humanization/Sessions/GatheringSessionManager.h`

```cpp
#pragma once

#include "Define.h"
#include "ActivitySession.h"
#include "Position.h"
#include <vector>

namespace Playerbot
{

/**
 * @brief Manages dedicated gathering sessions (30-60+ minutes)
 * 
 * Unlike the existing GatheringManager which gathers opportunistically,
 * this creates dedicated farming sessions where the bot:
 * - Stays in one zone
 * - Follows gathering routes
 * - Takes breaks naturally
 * - Continues until session ends or bags full
 */
class TC_GAME_API GatheringSessionManager
{
public:
    struct GatheringRoute
    {
        uint32 zoneId;
        std::vector<Position> waypoints;
        uint16 requiredSkill;
        GatheringNodeType nodeType;
    };
    
    struct GatheringSessionData
    {
        // Extends ActivitySession
        ActivitySession baseSession;
        
        // Gathering specific
        GatheringRoute* activeRoute = nullptr;
        size_t currentWaypointIndex = 0;
        uint32 nodesGathered = 0;
        uint32 totalItemsLooted = 0;
        uint32 skillPointsGained = 0;
        
        // Humanization
        uint32 routeRepetitions = 0;         // How many times route completed
        bool isWaitingForRespawn = false;    // Waiting for nodes to respawn
        Milliseconds respawnWaitTime{0};
    };
    
    explicit GatheringSessionManager(Player* bot, BotAI* ai);
    
    /**
     * @brief Start a dedicated gathering session
     * 
     * @param nodeType What to gather (MINING, HERB, etc.)
     * @param duration How long (default: 30-60 min based on config)
     * @param zoneId Specific zone (0 = auto-select)
     * @return true if session started
     */
    bool StartGatheringSession(
        GatheringNodeType nodeType,
        Milliseconds duration = Milliseconds(0),
        uint32 zoneId = 0);
    
    /**
     * @brief Update gathering session
     */
    void Update(uint32 diff);
    
    /**
     * @brief Check if in gathering session
     */
    bool IsInGatheringSession() const;
    
    /**
     * @brief Get session progress
     */
    float GetProgress() const;
    
    /**
     * @brief Force end session (bags full, emergency, etc.)
     */
    void EndSession();
    
    /**
     * @brief Should we take a gathering break?
     * (stretch, look around, etc.)
     */
    bool ShouldTakeBreak() const;
    
    /**
     * @brief Get recommended gathering zone for level/skill
     */
    uint32 GetRecommendedZone(GatheringNodeType nodeType) const;
    
private:
    void SelectGatheringRoute();
    void FollowRoute(uint32 diff);
    void HandleNodeGathered();
    void SimulateHumanBehavior(uint32 diff);
    
    Player* _bot;
    BotAI* _ai;
    
    std::unique_ptr<GatheringSessionData> _sessionData;
    std::vector<GatheringRoute> _knownRoutes;
    
    // Humanization
    Milliseconds _timeSinceLastBreak{0};
    Milliseconds _nextBreakAt{0};
    bool _isOnBreak = false;
};

} // namespace Playerbot
```

### Task 2.2: FishingSessionManager (10h)

**Core Feature**: Fishing als "Hobby" - 30+ Minuten an einem Spot

```cpp
/**
 * @brief Manages fishing as a relaxing hobby activity
 * 
 * Bots will:
 * - Find a nice fishing spot
 * - Fish for 30-60+ minutes
 * - Occasionally sit down
 * - Use emotes (yawn, stretch)
 * - Sometimes just watch the water
 */
class TC_GAME_API FishingSessionManager
{
public:
    struct FishingSpot
    {
        Position position;
        uint32 zoneId;
        uint32 areaId;
        bool isPool;                     // Fishing pool vs open water
        uint16 requiredSkill;
        std::vector<uint32> possibleFish; // Fish item IDs
    };
    
    struct FishingSessionData
    {
        ActivitySession baseSession;
        
        FishingSpot spot;
        uint32 fishCaught = 0;
        uint32 junkCaught = 0;
        uint32 raresFound = 0;
        
        // Humanization
        bool isSitting = false;
        bool isLookingAround = false;
        Milliseconds nextPostureChange{0};
        Milliseconds nextEmote{0};
    };
    
    /**
     * @brief Start a fishing session
     * 
     * @param duration How long (default: 30-60 min)
     * @param preferPools Prefer fishing pools
     */
    bool StartFishingSession(
        Milliseconds duration = Milliseconds(0),
        bool preferPools = false);
    
    void Update(uint32 diff);
    bool IsInFishingSession() const;
    void EndSession();
    
private:
    void FindFishingSpot();
    void CastFishingLine();
    void HandleCatch();
    void SimulateRelaxedBehavior(uint32 diff);
    void DoPostureChange();
    void DoFishingEmote();
    
    Player* _bot;
    BotAI* _ai;
    std::unique_ptr<FishingSessionData> _sessionData;
};
```

---

## Phase 3: City Life (35h)

### Task 3.1: CityLifeBehaviorManager (10h)

**File**: `Humanization/CityLife/CityLifeBehaviorManager.h`

```cpp
#pragma once

#include "Define.h"
#include "CityActivity.h"
#include <memory>
#include <vector>

namespace Playerbot
{

/**
 * @brief City activities for human-like town behavior
 */
enum class CityActivity : uint8
{
    NONE = 0,
    
    // Economic
    AH_BROWSING,            // Standing at AH, checking prices
    AH_POSTING,             // Posting items
    MAILBOX_CHECK,          // Checking mail
    BANK_VISIT,             // Visiting bank
    VENDOR_SHOPPING,        // Looking at vendors
    
    // Social
    INN_REST,               // Sitting in inn
    WANDERING,              // Walking around town
    SOCIALIZING,            // Emotes, "talking"
    WATCHING,               // Just standing and watching
    
    // Services
    TRAINER_VISIT,          // Visiting class trainer
    PROFESSION_TRAINER,     // Visiting profession trainer
    REPAIR,                 // Getting repairs
    
    MAX
};

/**
 * @brief Manages human-like behavior in cities
 * 
 * When a bot enters a city, instead of immediately doing business and leaving,
 * they will:
 * - Browse the AH for 10-30 minutes
 * - Check their mail
 * - Visit the inn
 * - Wander around
 * - Eventually leave for adventuring
 */
class TC_GAME_API CityLifeBehaviorManager
{
public:
    explicit CityLifeBehaviorManager(Player* bot, BotAI* ai);
    
    // ========================================================================
    // LIFECYCLE
    // ========================================================================
    
    void Update(uint32 diff);
    
    /**
     * @brief Start city life behaviors
     * Called when entering a city
     */
    void StartCityLife();
    
    /**
     * @brief End city life and prepare to leave
     */
    void EndCityLife();
    
    /**
     * @brief Check if in city life mode
     */
    bool IsInCityLife() const { return _isInCityLife; }
    
    // ========================================================================
    // ACTIVITY MANAGEMENT
    // ========================================================================
    
    /**
     * @brief Get current activity
     */
    CityActivity GetCurrentActivity() const { return _currentActivity; }
    
    /**
     * @brief Force specific activity
     */
    void SetActivity(CityActivity activity);
    
    /**
     * @brief Get time remaining in current activity
     */
    Milliseconds GetActivityTimeRemaining() const;
    
    // ========================================================================
    // SPECIFIC BEHAVIORS
    // ========================================================================
    
    /**
     * @brief Start AH browsing (10-30 min)
     */
    void StartAHBrowsing();
    
    /**
     * @brief Start inn rest (5-15 min)
     */
    void StartInnRest();
    
    /**
     * @brief Check mailbox (2-5 min)
     */
    void CheckMailbox();
    
    /**
     * @brief Visit trainer
     */
    void VisitTrainer();
    
private:
    void SelectNextActivity();
    void UpdateCurrentActivity(uint32 diff);
    void PerformActivityBehavior();
    
    // Activity-specific updates
    void UpdateAHBrowsing(uint32 diff);
    void UpdateInnRest(uint32 diff);
    void UpdateWandering(uint32 diff);
    void UpdateSocializing(uint32 diff);
    
    Player* _bot;
    BotAI* _ai;
    
    bool _isInCityLife = false;
    CityActivity _currentActivity = CityActivity::NONE;
    Milliseconds _activityTimeRemaining{0};
    Milliseconds _totalCityTime{0};
    Milliseconds _plannedCityDuration{0};
    
    // Activity history to ensure variety
    std::vector<CityActivity> _completedActivities;
    
    // Position tracking
    Position _ahPosition;
    Position _mailboxPosition;
    Position _innPosition;
    Position _trainerPosition;
};

} // namespace Playerbot
```

---

## Phase 4: Simulation (30h)

### Task 4.1: AFKSimulator (10h)

```cpp
/**
 * @brief Simulates AFK behavior for human-like play patterns
 * 
 * Real players:
 * - Take bathroom breaks
 * - Get food/drinks
 * - Answer phone/door
 * - Get distracted by real life
 * 
 * This simulates that behavior.
 */
class TC_GAME_API AFKSimulator
{
public:
    struct AFKEvent
    {
        Milliseconds duration;
        bool shouldSit;
        bool shouldUseAFKEmote;
        Position afkPosition;
        std::string reason;      // For logging
    };
    
    explicit AFKSimulator(Player* bot, BotAI* ai);
    
    void Update(uint32 diff);
    
    /**
     * @brief Check if should go AFK
     */
    bool ShouldGoAFK() const;
    
    /**
     * @brief Start AFK
     */
    void StartAFK();
    
    /**
     * @brief End AFK
     */
    void EndAFK();
    
    /**
     * @brief Is currently AFK
     */
    bool IsAFK() const { return _isAFK; }
    
    /**
     * @brief Get remaining AFK time
     */
    Milliseconds GetRemainingAFKTime() const;
    
private:
    void CalculateNextAFKTime();
    AFKEvent GenerateAFKEvent();
    void PerformAFKBehavior();
    
    Player* _bot;
    BotAI* _ai;
    
    bool _isAFK = false;
    AFKEvent _currentAFK;
    Milliseconds _afkTimeRemaining{0};
    Milliseconds _timeSinceLastAFK{0};
    Milliseconds _nextAFKAt{0};
};
```

---

## Integration mit BotAI

### Task: BotAI Integration (8h)

**In BotAI.h hinzufügen**:

```cpp
class HumanizationManager;

class TC_GAME_API BotAI : public IEventHandler<...>
{
public:
    // ... existing code ...
    
    // ========================================================================
    // HUMANIZATION SYSTEM
    // ========================================================================
    
    /**
     * @brief Get humanization manager
     */
    HumanizationManager* GetHumanization() { return _humanization.get(); }
    HumanizationManager const* GetHumanization() const { return _humanization.get(); }
    
    /**
     * @brief Check if should continue current activity
     * Returns false if session ended, AFK, etc.
     */
    bool ShouldContinueActivity() const;
    
    /**
     * @brief Get recommended activity type from humanization
     */
    ActivityType GetRecommendedActivity() const;
    
private:
    std::unique_ptr<HumanizationManager> _humanization;
};
```

**In BotAI::UpdateAI() modifizieren**:

```cpp
void BotAI::UpdateAI(uint32 diff)
{
    // ... existing checks ...
    
    // Humanization update (before activity decisions)
    if (_humanization)
    {
        _humanization->Update(diff);
        
        // Check if AFK
        if (_humanization->IsAFK())
        {
            // Don't do anything while AFK
            return;
        }
        
        // Check if should switch activity
        if (_humanization->ShouldSwitchActivity())
        {
            ActivityType nextActivity = _humanization->GetSessionManager()
                ->GetRecommendedNextActivity();
            // Handle activity switch...
        }
    }
    
    // ... rest of update ...
}
```

---

## Config Additions

**In playerbots.conf.dist hinzufügen**:

```ini
###############################################################################
# HUMANIZATION SETTINGS
###############################################################################

# Enable humanization system (human-like behavior)
Playerbot.Humanization.Enabled = true

# Session durations (milliseconds)
# Bots will stay in one activity for this duration before switching
Playerbot.Humanization.Session.MinDuration = 1800000
Playerbot.Humanization.Session.MaxDuration = 5400000

# Gathering sessions (30-60 minutes minimum!)
Playerbot.Humanization.Gathering.MinDuration = 1800000
Playerbot.Humanization.Gathering.MaxDuration = 3600000

# Fishing sessions (hobby fishing, 30+ minutes)
Playerbot.Humanization.Fishing.MinDuration = 1800000
Playerbot.Humanization.Fishing.MaxDuration = 3600000

# City life (time spent in cities doing non-essential activities)
Playerbot.Humanization.CityLife.Enabled = true
Playerbot.Humanization.CityLife.MinDuration = 600000
Playerbot.Humanization.CityLife.MaxDuration = 1800000
Playerbot.Humanization.CityLife.AHBrowsingDuration = 900000
Playerbot.Humanization.CityLife.InnRestChance = 0.3

# AFK simulation
Playerbot.Humanization.AFK.Enabled = true
Playerbot.Humanization.AFK.MinInterval = 1800000
Playerbot.Humanization.AFK.MaxInterval = 3600000
Playerbot.Humanization.AFK.MinDuration = 60000
Playerbot.Humanization.AFK.MaxDuration = 300000

# Error simulation (makes bots imperfect like humans)
Playerbot.Humanization.Errors.Enabled = true
Playerbot.Humanization.Errors.WrongPathChance = 0.05
Playerbot.Humanization.Errors.DistractionChance = 0.10
```

---

## Zusammenfassung

### Lieferungen

| Phase | Komponenten | Effort |
|-------|-------------|--------|
| Phase 1 | HumanizationManager, ActivitySession, PersonalityProfile, Config | 40h |
| Phase 2 | GatheringSessionManager (30+ Min), FishingSessionManager | 50h |
| Phase 3 | CityLifeBehaviorManager, AH/Inn/Mailbox Behaviors | 35h |
| Phase 4 | AFKSimulator, HumanErrorSimulator | 30h |
| Phase 5 | ActivityScheduler, Integration | 25h |
| Phase 6 | Personality Profiles | 20h |
| **Total** | | **200h** |

### Priorität

**P0 - Sofort**:
1. HumanizationManager Framework
2. GatheringSessionManager (30+ Minuten!)
3. CityLifeBehaviorManager

**P1 - Nächste Woche**:
4. FishingSessionManager  
5. AFKSimulator
6. InnBehavior

**P2 - Später**:
7. PersonalityProfiles
8. ErrorSimulation
9. DayNightScheduling
