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
#include <array>
#include <atomic>
#include <mutex>

class Player;
class Unit;

namespace Playerbot
{

// Player behavior patterns
enum class PlayerArchetype : uint8
{
    AGGRESSIVE,      // High damage, forward positioning
    DEFENSIVE,       // Conservative play, survival focus
    SUPPORTIVE,      // Healing/buffing allies
    TACTICAL,        // CC and positioning focus
    OPPORTUNISTIC,   // Target switching, burst windows
    CONSISTENT,      // Steady damage, predictable
    ADAPTIVE,        // Changes style frequently
    UNKNOWN
};

// Action pattern types
enum class PatternType : uint8
{
    MOVEMENT,        // Movement patterns
    ABILITY_USAGE,   // Spell/ability rotation
    TARGET_SELECTION,// Target prioritization
    POSITIONING,     // Spatial positioning
    RESOURCE_MGMT,   // Resource management
    REACTION_TIME,   // Response timing
    COMBO_SEQUENCE,  // Ability combinations
    DEFENSIVE_REACT  // Defensive responses
};

// Time-series data point for pattern analysis
struct BehaviorSample
{
    uint64_t timestamp;
    float x, y, z;              // Position
    float orientation;          // Facing direction
    uint32_t spellId;          // Spell cast (0 if none)
    ObjectGuid targetGuid;      // Current target
    float healthPct;           // Health percentage
    float resourcePct;         // Mana/energy percentage
    uint32_t auraCount;        // Active aura count
    bool isMoving;
    bool isInCombat;
    float damageDealt;         // Damage in last sample window
    float damageTaken;         // Damage taken in window
    float healingDone;         // Healing in window

    BehaviorSample() : timestamp(0), x(0), y(0), z(0), orientation(0),
        spellId(0), healthPct(100), resourcePct(100), auraCount(0),
        isMoving(false), isInCombat(false), damageDealt(0),
        damageTaken(0), healingDone(0) {}
};

// Pattern signature for matching
struct PatternSignature
{
    PatternType type;
    ::std::vector<float> features;      // Feature vector
    float confidence;                  // Recognition confidence
    uint32_t occurrences;             // How often seen
    ::std::chrono::steady_clock::time_point lastSeen;

    float CalculateSimilarity(const ::std::vector<float>& other) const;
};

// Player behavior profile
class TC_GAME_API PlayerProfile
{
public:
    PlayerProfile(ObjectGuid guid);
    ~PlayerProfile();

    // Profile management
    void AddSample(const BehaviorSample& sample);
    void UpdateArchetype();
    void Reset();

    // Pattern analysis
    PlayerArchetype GetArchetype() const { return _archetype; }
    float GetArchetypeConfidence() const { return _archetypeConfidence; }
    ::std::vector<PatternSignature> GetPatterns(PatternType type) const;

    // Behavior prediction
    uint32_t PredictNextSpell() const;
    Position PredictNextPosition(float deltaTime) const;
    ObjectGuid PredictNextTarget(const ::std::vector<Unit*>& candidates) const;
    float PredictReactionTime() const;

    // Statistics
    float GetAverageAPM() const { return _averageAPM; }  // Actions per minute
    float GetMovementVariance() const { return _movementVariance; }
    float GetTargetSwitchRate() const { return _targetSwitchRate; }
    float GetDefensiveReactivity() const { return _defensiveReactivity; }

    // Similarity comparison
    float CalculateSimilarity(const PlayerProfile& other) const;

private:
    // Profile data
    ObjectGuid _playerGuid;
    PlayerArchetype _archetype;
    float _archetypeConfidence;
    ::std::deque<BehaviorSample> _samples;
    static constexpr size_t MAX_SAMPLES = 1000;

    // Pattern storage
    ::std::unordered_map<PatternType, ::std::vector<PatternSignature>> _patterns;

    // Behavioral statistics
    ::std::atomic<float> _averageAPM;
    ::std::atomic<float> _movementVariance;
    ::std::atomic<float> _targetSwitchRate;
    ::std::atomic<float> _defensiveReactivity;
    ::std::atomic<float> _aggressionLevel;
    ::std::atomic<float> _survivalPriority;

    // Spell usage tracking
    ::std::unordered_map<uint32_t, uint32_t> _spellUsageCounts;
    ::std::vector<::std::pair<uint32_t, uint32_t>> _spellSequences;  // Spell ID pairs

    // Movement analysis
    ::std::vector<::std::array<float, 3>> _movementVectors;
    float _averageSpeed;
    float _positionEntropy;  // Randomness of movement

    // Combat metrics
    uint32_t _combatEngagements;
    uint32_t _combatVictories;
    float _averageCombatDuration;
    float _damageEfficiency;  // Damage per resource spent

    // Helper methods
    void ExtractMovementPatterns();
    void ExtractAbilityPatterns();
    void ExtractTargetingPatterns();
    void CalculateBehaviorMetrics();
    PlayerArchetype ClassifyArchetype() const;
};

// Clustering algorithm for player grouping
class TC_GAME_API BehaviorCluster
{
public:
    BehaviorCluster();
    ~BehaviorCluster();

    // Cluster management
    void AddProfile(::std::shared_ptr<PlayerProfile> profile);
    void RemoveProfile(ObjectGuid guid);
    void UpdateClusters();

    // Cluster analysis
    uint32_t GetClusterCount() const { return _clusters.size(); }
    ::std::vector<::std::shared_ptr<PlayerProfile>> GetCluster(uint32_t clusterId) const;
    uint32_t GetPlayerCluster(ObjectGuid guid) const;
    ::std::shared_ptr<PlayerProfile> GetClusterCentroid(uint32_t clusterId) const;

    // Similarity search
    ::std::vector<::std::shared_ptr<PlayerProfile>> FindSimilarProfiles(
        const PlayerProfile& profile, uint32_t maxCount = 5) const;

private:
    struct Cluster
    {
        ::std::vector<::std::shared_ptr<PlayerProfile>> members;
        ::std::shared_ptr<PlayerProfile> centroid;
        float inertia;  // Sum of squared distances to centroid
    };

    ::std::vector<Cluster> _clusters;
    ::std::unordered_map<ObjectGuid, uint32_t> _profileToCluster;
    ::std::unordered_map<ObjectGuid, ::std::shared_ptr<PlayerProfile>> _profiles;

    // K-means parameters
    uint32_t _k = 7;  // Number of clusters (matching archetypes)
    uint32_t _maxIterations = 100;
    float _convergenceThreshold = 0.01f;

    // Clustering methods
    void InitializeCentroids();
    void AssignToClusters();
    void UpdateCentroids();
    bool HasConverged() const;
};

// Main pattern recognition system
class TC_GAME_API PlayerPatternRecognition
{
public:
    static PlayerPatternRecognition& Instance()
    {
        static PlayerPatternRecognition instance;
        return instance;
    }

    // System initialization
    bool Initialize();
    void Shutdown();

    // Profile management
    void CreateProfile(Player* player);
    void DeleteProfile(ObjectGuid guid);
    ::std::shared_ptr<PlayerProfile> GetProfile(ObjectGuid guid) const;

    // Real-time tracking
    void RecordPlayerBehavior(Player* player);
    void RecordCombatAction(Player* player, uint32_t spellId, Unit* target);
    void RecordMovement(Player* player);
    void RecordTargetChange(Player* player, Unit* oldTarget, Unit* newTarget);

    // Pattern analysis
    void AnalyzePatterns(ObjectGuid guid);
    void UpdateAllPatterns();

    // Bot behavior mimicry
    void ApplyPlayerStyle(Player* bot, ObjectGuid templatePlayerGuid);
    void ApplyArchetypeStyle(Player* bot, PlayerArchetype archetype);
    ::std::string SelectActionBasedOnPattern(Player* bot, const PlayerProfile& pattern);

    // Prediction
    struct PredictionResult
    {
        float confidence;
        uint32_t predictedAction;
        Position predictedPosition;
        ObjectGuid predictedTarget;
        float predictedTiming;
    };

    PredictionResult PredictPlayerAction(Player* player, float timeHorizon = 1.0f);

    // Learning from players
    void LearnFromSuccessfulAction(Player* player, const ::std::string& action, float effectiveness);
    void LearnFromCombatOutcome(Player* player, bool victory, float performance);

    // Statistics and metrics
    struct RecognitionMetrics
    {
        ::std::atomic<uint32_t> profilesTracked{0};
        ::std::atomic<uint32_t> patternsRecognized{0};
        ::std::atomic<float> averageConfidence{0.0f};
        ::std::atomic<float> predictionAccuracy{0.0f};
        ::std::atomic<uint64_t> samplesProcessed{0};
    };

    RecognitionMetrics GetMetrics() const { return _metrics; }

    // Anomaly detection
    bool IsAnomalousBehavior(Player* player) const;
    float GetAnomalyScore(Player* player) const;
    ::std::vector<::std::string> DetectExploits(Player* player) const;

    // Meta-game analysis
    void UpdateMetaPatterns();
    ::std::vector<PatternSignature> GetCurrentMetaPatterns() const;
    void AdaptBotsToMeta();

    // Performance optimization
    void OptimizePatternStorage();
    void PruneOldPatterns(uint32_t olderThanHours = 48);

private:
    PlayerPatternRecognition();
    ~PlayerPatternRecognition();

    // System state
    bool _initialized;
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE> _profilesMutex;
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE> _clusterMutex;

    // Profile storage
    ::std::unordered_map<ObjectGuid, ::std::shared_ptr<PlayerProfile>> _profiles;
    ::std::unique_ptr<BehaviorCluster> _behaviorCluster;

    // Pattern database
    ::std::vector<PatternSignature> _globalPatterns;
    ::std::unordered_map<PlayerArchetype, ::std::vector<PatternSignature>> _archetypePatterns;

    // Meta patterns
    ::std::vector<PatternSignature> _metaPatterns;
    ::std::chrono::steady_clock::time_point _lastMetaUpdate;

    // Metrics
    mutable RecognitionMetrics _metrics;

    // Prediction validation
    struct PredictionValidation
    {
        ObjectGuid playerGuid;
        PredictionResult prediction;
        ::std::chrono::steady_clock::time_point timestamp;
        bool validated;
        float accuracy;
    };
    ::std::deque<PredictionValidation> _predictionHistory;
    static constexpr size_t MAX_PREDICTION_HISTORY = 1000;

    // Helper methods
    BehaviorSample CreateBehaviorSample(Player* player) const;
    ::std::vector<float> ExtractFeatureVector(const BehaviorSample& sample) const;
    void UpdatePredictionAccuracy();
    void PropagateSuccessfulPatterns();
    void ClusterProfiles();
    float CalculatePatternStrength(const PatternSignature& pattern) const;

    // Pattern detection algorithms
    ::std::vector<PatternSignature> DetectRepeatingSequences(
        const ::std::deque<BehaviorSample>& samples) const;
    ::std::vector<PatternSignature> DetectMovementPatterns(
        const ::std::deque<BehaviorSample>& samples) const;
    ::std::vector<PatternSignature> DetectCombatRotations(
        const ::std::deque<BehaviorSample>& samples) const;

    // Constants
    static constexpr uint32_t MIN_SAMPLES_FOR_PATTERN = 10;
    static constexpr float MIN_PATTERN_CONFIDENCE = 0.7f;
    static constexpr uint32_t CLUSTERING_INTERVAL_MS = 60000;  // 1 minute
    static constexpr uint32_t PATTERN_UPDATE_INTERVAL_MS = 5000;  // 5 seconds
    static constexpr float ANOMALY_THRESHOLD = 3.0f;  // Standard deviations
};

// RAII helper for scoped pattern recording
class TC_GAME_API ScopedPatternRecording
{
public:
    ScopedPatternRecording(Player* player);
    ~ScopedPatternRecording();

    void RecordAction(uint32_t actionId);
    void RecordPosition();
    void RecordCombatMetric(float damage, float healing);

private:
    Player* _player;
    ::std::vector<BehaviorSample> _samples;
    ::std::chrono::steady_clock::time_point _startTime;
};

#define sPlayerPatternRecognition PlayerPatternRecognition::Instance()

} // namespace Playerbot