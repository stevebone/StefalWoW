/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 */

#ifndef BOT_AI_INITIALIZER_H
#define BOT_AI_INITIALIZER_H

#include "Define.h"
#include "Threading/LockHierarchy.h"
#include "Player.h"
#include <memory>
#include <functional>
#include <chrono>

namespace Playerbot {

// Forward declarations
class BotAI;
class Strategy;
class Action;

/**
 * Bot AI Initialization States
 */
enum class AIInitState : uint8
{
    UNINITIALIZED,      // AI not yet initialized
    LOADING_CONFIG,     // Loading AI configuration
    CREATING_AI,        // Creating AI instance
    LOADING_STRATEGIES, // Loading strategy patterns
    LOADING_ACTIONS,    // Loading action definitions
    LOADING_VALUES,     // Loading value calculators
    CONFIGURING,        // Configuring AI parameters
    CALIBRATING,        // Calibrating for player level/gear
    READY,              // AI ready but not active
    ACTIVE,             // AI fully active
    ERROR               // Initialization failed
};

/**
 * AI Configuration based on bot role and purpose
 */
struct BotAIConfig
{
    enum Role
    {
        TANK,
        HEALER,
        DPS_MELEE,
        DPS_RANGED,
        DPS_CASTER,
        SUPPORT,
        HYBRID,
        AUTO_DETECT     // Detect based on class/spec
    };

    enum Behavior
    {
        AGGRESSIVE,     // Actively seeks combat
        DEFENSIVE,      // Focuses on survival
        SUPPORTIVE,     // Helps allies
        PASSIVE,        // Minimal actions
        ADAPTIVE        // Adjusts based on situation
    };

    enum Difficulty
    {
        BEGINNER,       // Simple rotations, slower reactions
        NORMAL,         // Standard performance
        ADVANCED,       // Optimal rotations, quick reactions
        EXPERT          // Near-perfect play
    };

    Role role = AUTO_DETECT;
    Behavior behavior = ADAPTIVE;
    Difficulty difficulty = NORMAL;

    // Performance tuning
    uint32 reactionTimeMs = 500;        // Base reaction time
    uint32 decisionIntervalMs = 100;    // How often to make decisions
    float skillVariance = 0.1f;         // 0-1 variance in skill execution

    // Resource management
    float manaConservationThreshold = 0.3f;  // Start conserving at 30% mana
    float healthPanicThreshold = 0.2f;       // Panic mode at 20% health

    // Combat settings
    bool useConsumables = true;
    bool useCooldowns = true;
    bool avoidAOE = true;
    float threatModifier = 1.0f;        // Threat generation modifier

    // Social settings
    bool respondToChat = true;
    bool followLeader = true;
    bool assistOthers = true;
    float followDistance = 5.0f;
};

/**
 * Bot AI Initializer
 *
 * Handles the complete initialization sequence for bot AI systems
 */
class TC_GAME_API BotAIInitializer
{
public:
    using InitCallback = ::std::function<void(bool success, BotAI* ai)>;

    BotAIInitializer(Player* bot);
    ~BotAIInitializer();

    /**
     * Initialize AI with default configuration
     * @param callback Optional callback when complete
     * @return true if initialization started
     */
    bool Initialize(InitCallback callback = nullptr);

    /**
     * Initialize AI with custom configuration
     * @param config Custom AI configuration
     * @param callback Optional callback when complete
     * @return true if initialization started
     */
    bool Initialize(BotAIConfig const& config, InitCallback callback = nullptr);

    /**
     * Process initialization steps
     * @param diff Time since last update
     * @return true if still initializing
     */
    bool Process(uint32 diff);

    /**
     * Get current initialization state
     */
    AIInitState GetState() const { return _state; }

    /**
     * Check if initialization is complete
     */
    bool IsReady() const { return _state == AIInitState::READY || _state == AIInitState::ACTIVE; }

    /**
     * Check if AI failed to initialize
     */
    bool HasFailed() const { return _state == AIInitState::ERROR; }

    /**
     * Activate the AI (start processing)
     */
    bool Activate();

    /**
     * Deactivate the AI (stop processing)
     */
    void Deactivate();

    /**
     * Get the initialized AI instance
     */
    BotAI* GetAI() const { return _ai; }

    /**
     * Get initialization error message
     */
    ::std::string GetError() const { return _errorMessage; }

private:
    // === Initialization Steps ===

    bool LoadConfiguration();
    bool CreateAIInstance();
    bool LoadStrategies();
    bool LoadActions();
    bool LoadValues();
    bool ConfigureAI();
    bool CalibrateAI();
    bool FinalizeInitialization();

    // === Configuration Helpers ===

    BotAIConfig DetermineAutoConfig() const;
    BotAIConfig::Role DetectRole() const;
    void ApplyDifficultySettings();
    void ConfigureClassSpecificSettings();

    // === Strategy Loading ===

    void LoadCombatStrategies();
    void LoadMovementStrategies();
    void LoadQuestStrategies();
    void LoadSocialStrategies();

    // === Action Loading ===

    void LoadCombatActions();
    void LoadHealingActions();
    void LoadMovementActions();
    void LoadItemActions();
    void LoadQuestActions();

    // === Calibration ===

    void CalibrateForLevel();
    void CalibrateForGear();
    void CalibrateForGroup();

    // === Error Handling ===

    void SetError(::std::string const& error);
    void HandleInitializationFailure();

private:
    // Core components
    Player* _bot;
    BotAI* _ai;
    BotAIConfig _config;

    // State management
    AIInitState _state;
    ::std::chrono::steady_clock::time_point _stateStartTime;

    // Callback management
    InitCallback _callback;

    // Error tracking
    ::std::string _errorMessage;
    uint32 _retryCount;
    static constexpr uint32 MAX_RETRIES = 3;

    // Timing
    static constexpr uint32 STATE_TIMEOUT_MS = 5000;
};

/**
 * AI Activation Controller
 *
 * Manages AI activation and deactivation with proper state transitions
 */
class TC_GAME_API BotAIActivation
{
public:
    /**
     * Activate AI for a bot
     * @param bot The bot player
     * @param config Optional AI configuration
     * @return AI instance if successful, nullptr otherwise
     */
    static BotAI* ActivateBot(Player* bot, BotAIConfig const* config = nullptr);

    /**
     * Deactivate AI for a bot
     * @param bot The bot player
     */
    static void DeactivateBot(Player* bot);

    /**
     * Check if bot has active AI
     * @param bot The bot player
     * @return true if AI is active
     */
    static bool IsActive(Player* bot);

    /**
     * Reconfigure active AI
     * @param bot The bot player
     * @param config New configuration
     * @return true if reconfigured successfully
     */
    static bool ReconfigureBot(Player* bot, BotAIConfig const& config);

    /**
     * Reset AI to default state
     * @param bot The bot player
     */
    static void ResetBot(Player* bot);

    /**
     * Emergency stop all bot AIs
     */
    static void EmergencyStopAll();

private:
    // Track active AIs
    static ::std::unordered_map<ObjectGuid, BotAI*> _activeAIs;
    static Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE> _aiMutex;
};

/**
 * Pre-configured AI templates for common scenarios
 */
class TC_GAME_API BotAITemplates
{
public:
    // Role-based templates
    static BotAIConfig GetTankTemplate();
    static BotAIConfig GetHealerTemplate();
    static BotAIConfig GetMeleeDPSTemplate();
    static BotAIConfig GetRangedDPSTemplate();
    static BotAIConfig GetCasterDPSTemplate();

    // Behavior templates
    static BotAIConfig GetQuestingTemplate();
    static BotAIConfig GetDungeonTemplate();
    static BotAIConfig GetRaidTemplate();
    static BotAIConfig GetPvPTemplate();
    static BotAIConfig GetGatheringTemplate();

    // Difficulty templates
    static BotAIConfig GetBeginnerTemplate();
    static BotAIConfig GetNormalTemplate();
    static BotAIConfig GetAdvancedTemplate();
    static BotAIConfig GetExpertTemplate();

    // Class-specific templates
    static BotAIConfig GetWarriorTemplate(bool tank = false);
    static BotAIConfig GetPaladinTemplate(bool tank = false, bool healer = false);
    static BotAIConfig GetHunterTemplate();
    static BotAIConfig GetRogueTemplate();
    static BotAIConfig GetPriestTemplate(bool healer = true);
    static BotAIConfig GetDeathKnightTemplate(bool tank = false);
    static BotAIConfig GetShamanTemplate(bool healer = false);
    static BotAIConfig GetMageTemplate();
    static BotAIConfig GetWarlockTemplate();
    static BotAIConfig GetMonkTemplate(bool tank = false, bool healer = false);
    static BotAIConfig GetDruidTemplate(bool tank = false, bool healer = false);
    static BotAIConfig GetDemonHunterTemplate(bool tank = false);
    static BotAIConfig GetEvokerTemplate(bool healer = false);
};

} // namespace Playerbot

#endif // BOT_AI_INITIALIZER_H