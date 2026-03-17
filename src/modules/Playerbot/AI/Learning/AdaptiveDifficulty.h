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
#include "Threading/LockHierarchy.h"
#include "ObjectGuid.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <deque>
#include <chrono>
#include <atomic>
#include <mutex>

class Player;
class Unit;

namespace Playerbot
{

// Forward declarations
class BotAI;

// Difficulty dimensions
enum class DifficultyAspect : uint8
{
    REACTION_TIME,      // How quickly bot responds
    ACCURACY,          // Hit rate and skill usage accuracy
    AGGRESSION,        // How aggressive the playstyle
    COOPERATION,       // Group coordination level
    RESOURCE_MGMT,     // Resource efficiency
    POSITIONING,       // Movement and positioning quality
    ADAPTATION_SPEED,  // How quickly bot adapts to player
    OVERALL           // Combined difficulty
};

// Player skill indicators
struct SkillIndicators
{
    float accuracy;           // Player hit rate
    float reactionTime;       // Average reaction time
    float apm;               // Actions per minute
    float survivalRate;      // How often player survives encounters
    float damageEfficiency;  // Damage per resource
    float positioningQuality;// Movement efficiency
    float decisionQuality;   // Quality of tactical decisions
    float learningRate;      // How quickly player improves

    SkillIndicators() : accuracy(0.5f), reactionTime(1.0f), apm(30.0f),
        survivalRate(0.5f), damageEfficiency(0.5f), positioningQuality(0.5f),
        decisionQuality(0.5f), learningRate(0.0f) {}

    float GetOverallSkill() const;
};

// Performance tracking for difficulty adjustment
struct PerformanceWindow
{
    uint64_t startTime;
    uint64_t endTime;
    uint32_t playerWins;
    uint32_t botWins;
    uint32_t draws;
    float averageEngagementDuration;
    float playerSatisfactionScore;  // Derived from various metrics

    PerformanceWindow() : startTime(0), endTime(0), playerWins(0),
        botWins(0), draws(0), averageEngagementDuration(0),
        playerSatisfactionScore(0.5f) {}

    float GetWinRate() const;
    float GetBalance() const;  // How balanced the matches are
};

// Difficulty curve for smooth adjustment
class TC_GAME_API DifficultyCurve
{
public:
    DifficultyCurve(float initialDifficulty = 0.5f);
    ~DifficultyCurve();

    // Curve management
    void AddDataPoint(float playerSkill, float optimalDifficulty);
    float GetDifficulty(float playerSkill) const;
    void Smooth(float smoothingFactor = 0.1f);
    void Reset();

    // Curve properties
    float GetSlope() const;
    float GetIntercept() const;
    bool IsTrained() const { return _dataPoints.size() >= MIN_POINTS_FOR_TRAINING; }

private:
    struct DataPoint
    {
        float skill;
        float difficulty;
        float weight;  // How much this point influences the curve
    };

    ::std::vector<DataPoint> _dataPoints;
    float _slope;
    float _intercept;
    bool _fitted;

    static constexpr size_t MIN_POINTS_FOR_TRAINING = 10;
    static constexpr size_t MAX_DATA_POINTS = 100;

    void FitCurve();  // Linear regression or polynomial fitting
    float Interpolate(float skill) const;
};

// Player skill profile for difficulty matching
class TC_GAME_API PlayerSkillProfile
{
public:
    PlayerSkillProfile(ObjectGuid playerGuid);
    ~PlayerSkillProfile();

    // Skill assessment
    void UpdateSkillIndicators(const SkillIndicators& indicators);
    SkillIndicators GetCurrentSkill() const { return _currentSkill; }
    float GetSkillLevel() const;  // 0.0 to 1.0
    float GetSkillTrend() const;  // Rate of improvement

    // Performance tracking
    void RecordEngagement(bool playerWon, float duration);
    void RecordAction(bool successful, float reactionTime);
    PerformanceWindow GetRecentPerformance() const;

    // Difficulty recommendation
    float GetRecommendedDifficulty() const;
    float GetDifficultyAdjustmentRate() const;

    // Frustration and engagement detection
    float GetFrustrationLevel() const;
    float GetEngagementLevel() const;
    bool NeedsDifficultyAdjustment() const;

private:
    ObjectGuid _playerGuid;
    SkillIndicators _currentSkill;
    ::std::deque<SkillIndicators> _skillHistory;
    ::std::deque<PerformanceWindow> _performanceHistory;

    // Metrics
    ::std::atomic<float> _skillLevel;
    ::std::atomic<float> _frustrationLevel;
    ::std::atomic<float> _engagementLevel;
    ::std::atomic<uint32_t> _totalEngagements;
    ::std::atomic<uint32_t> _consecutiveWins;
    ::std::atomic<uint32_t> _consecutiveLosses;

    // Timing
    ::std::chrono::steady_clock::time_point _lastUpdate;
    ::std::chrono::steady_clock::time_point _profileCreated;

    static constexpr size_t MAX_HISTORY_SIZE = 50;
    static constexpr uint32_t FRUSTRATION_THRESHOLD = 5;  // Consecutive losses
    static constexpr uint32_t BOREDOM_THRESHOLD = 5;      // Consecutive wins

    void CalculateFrustration();
    void CalculateEngagement();
    void UpdateSkillLevel();
};

// Dynamic difficulty adjustment system
class TC_GAME_API AdaptiveDifficulty
{
public:
    static AdaptiveDifficulty& Instance()
    {
        static AdaptiveDifficulty instance;
        return instance;
    }

    // System initialization
    bool Initialize();
    void Shutdown();
    bool IsEnabled() const { return _enabled; }

    // Player profile management
    void CreatePlayerProfile(Player* player);
    void DeletePlayerProfile(ObjectGuid guid);
    ::std::shared_ptr<PlayerSkillProfile> GetPlayerProfile(ObjectGuid guid) const;

    // Skill assessment
    void AssessPlayerSkill(Player* player);
    SkillIndicators CalculateSkillIndicators(Player* player) const;
    void UpdatePlayerSkill(Player* player, const SkillIndicators& indicators);

    // Bot difficulty adjustment
    void AdjustBotDifficulty(BotAI* bot, Player* opponent);
    void SetBotDifficulty(BotAI* bot, float difficulty);
    float GetBotDifficulty(BotAI* bot) const;

    // Difficulty parameters
    struct DifficultySettings
    {
        float reactionTimeMultiplier;    // 0.5 = twice as fast, 2.0 = twice as slow
        float accuracyModifier;          // -0.5 to 0.5 added to hit chance
        float damageModifier;            // 0.5 to 1.5 damage multiplier
        float healthModifier;            // 0.5 to 1.5 health multiplier
        float aggressionLevel;           // 0.0 to 1.0 aggression
        float cooperationLevel;          // 0.0 to 1.0 group coordination
        float adaptationSpeed;           // How quickly bot learns player patterns
        float resourceEfficiency;        // How efficiently bot uses resources
        float positioningQuality;        // Movement and positioning accuracy
        float abilityUsageOptimization;  // How optimal ability usage is

        DifficultySettings();
        void ApplyDifficulty(float difficulty);
        void ApplyAspect(DifficultyAspect aspect, float value);
    };

    DifficultySettings CalculateDifficultySettings(float playerSkill, float targetDifficulty);

    // Real-time adjustment
    void MonitorEngagement(Player* player, BotAI* bot);
    void RecordCombatOutcome(Player* player, BotAI* bot, bool playerWon, float duration);
    void AdjustDifficultyInCombat(BotAI* bot, Player* opponent);

    // Group difficulty balancing
    void BalanceGroupDifficulty(const ::std::vector<Player*>& players,
                                const ::std::vector<BotAI*>& bots);
    float CalculateGroupSkillLevel(const ::std::vector<Player*>& players) const;

    // Difficulty curves
    void TrainDifficultyCurve(ObjectGuid playerGuid, float skill, float optimalDifficulty);
    float GetOptimalDifficulty(ObjectGuid playerGuid, float currentSkill) const;

    // Flow state optimization
    bool IsInFlowState(Player* player) const;
    float GetFlowStateScore(Player* player) const;
    void OptimizeForFlow(BotAI* bot, Player* player);

    // Configuration
    void SetDifficultyMode(const ::std::string& mode);  // "adaptive", "fixed", "progressive"
    void SetAdjustmentSpeed(float speed) { _adjustmentSpeed = ::std::clamp(speed, 0.01f, 1.0f); }
    void SetTargetWinRate(float rate) { _targetWinRate = ::std::clamp(rate, 0.3f, 0.7f); }

    // Metrics
    struct DifficultyMetrics
    {
        ::std::atomic<uint32_t> profilesTracked{0};
        ::std::atomic<uint32_t> adjustmentsMade{0};
        ::std::atomic<float> averagePlayerSatisfaction{0.0f};
        ::std::atomic<float> averageSkillMatch{0.0f};
        ::std::atomic<uint32_t> flowStatesAchieved{0};
    };

    DifficultyMetrics GetMetrics() const { return _metrics; }

    // Difficulty presets
    enum class DifficultyPreset : uint8
    {
        BEGINNER,
        EASY,
        NORMAL,
        HARD,
        EXPERT,
        ADAPTIVE
    };

    void ApplyPreset(BotAI* bot, DifficultyPreset preset);
    DifficultySettings GetPresetSettings(DifficultyPreset preset) const;

private:
    AdaptiveDifficulty();
    ~AdaptiveDifficulty();

    // System state
    bool _initialized;
    bool _enabled;
    ::std::string _difficultyMode;
    float _adjustmentSpeed;
    float _targetWinRate;

    // Player profiles
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE> _profilesMutex;
    ::std::unordered_map<ObjectGuid, ::std::shared_ptr<PlayerSkillProfile>> _playerProfiles;

    // Bot difficulty settings
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE> _botDifficultyMutex;
    ::std::unordered_map<uint32_t, DifficultySettings> _botDifficulties;

    // Difficulty curves
    ::std::unordered_map<ObjectGuid, ::std::unique_ptr<DifficultyCurve>> _difficultyCurves;

    // Metrics
    mutable DifficultyMetrics _metrics;

    // Helper methods
    ::std::shared_ptr<PlayerSkillProfile> GetOrCreateProfile(ObjectGuid guid);
    void UpdateFlowState(Player* player);
    float CalculateDifficultyDelta(float currentWinRate, float targetWinRate) const;
    DifficultySettings InterpolateDifficulty(const DifficultySettings& easy,
                                            const DifficultySettings& hard,
                                            float t) const;

    // Analysis methods
    float AnalyzePlayerBehavior(Player* player) const;
    float PredictOptimalChallenge(PlayerSkillProfile* profile) const;
    void AdaptToPlayerLearning(PlayerSkillProfile* profile, DifficultySettings& settings);

    // Player action tracking for skill calculation
    struct PlayerActionTracker
    {
        // Spell cast tracking
        uint32 spellsCastTotal{0};
        uint32 spellsHitTotal{0};
        uint32 spellsMissed{0};
        uint32 spellsInterrupted{0};

        // Timing tracking
        uint32 lastActionTime{0};
        uint32 actionCount{0};
        uint32 trackingStartTime{0};
        float totalReactionTime{0.0f};
        uint32 reactionSamples{0};

        // Resource usage
        uint32 resourceWasted{0};
        uint32 resourceUsed{0};

        // Position tracking
        float totalMovementDistance{0.0f};
        float optimalMovementDistance{0.0f};
        uint32 goodPositionCount{0};
        uint32 positionChecks{0};

        // Decision tracking
        uint32 goodDecisions{0};
        uint32 totalDecisions{0};

        void Reset()
        {
            spellsCastTotal = 0;
            spellsHitTotal = 0;
            spellsMissed = 0;
            spellsInterrupted = 0;
            lastActionTime = 0;
            actionCount = 0;
            trackingStartTime = 0;
            totalReactionTime = 0.0f;
            reactionSamples = 0;
            resourceWasted = 0;
            resourceUsed = 0;
            totalMovementDistance = 0.0f;
            optimalMovementDistance = 0.0f;
            goodPositionCount = 0;
            positionChecks = 0;
            goodDecisions = 0;
            totalDecisions = 0;
        }
    };

    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE> _trackerMutex;
    ::std::unordered_map<ObjectGuid, PlayerActionTracker> _playerTrackers;

    // Constants
    static constexpr float MIN_DIFFICULTY = 0.0f;
    static constexpr float MAX_DIFFICULTY = 1.0f;
    static constexpr float DEFAULT_DIFFICULTY = 0.5f;
    static constexpr float FLOW_STATE_THRESHOLD = 0.8f;
    static constexpr uint32_t ADJUSTMENT_COOLDOWN_MS = 5000;  // 5 seconds
    static constexpr float MAX_ADJUSTMENT_PER_STEP = 0.1f;
};

// RAII helper for difficulty session
class TC_GAME_API ScopedDifficultyAdjustment
{
public:
    ScopedDifficultyAdjustment(BotAI* bot, Player* player);
    ~ScopedDifficultyAdjustment();

    void RecordPlayerAction(bool successful, float reactionTime);
    void RecordBotAction(bool successful);
    void MarkCombatEnd(bool playerWon);

private:
    BotAI* _bot;
    Player* _player;
    float _initialDifficulty;
    ::std::chrono::steady_clock::time_point _startTime;
    uint32_t _playerSuccesses;
    uint32_t _botSuccesses;
    float _totalReactionTime;
    uint32_t _reactionSamples;
};

#define sAdaptiveDifficulty AdaptiveDifficulty::Instance()

} // namespace Playerbot