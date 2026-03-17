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
#include <array>
#include <chrono>
#include <atomic>
#include <deque>
#include <random>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <mutex>

class Player;
class Unit;

namespace Playerbot
{

// Forward declarations
class BotAI;
class Action;
struct ActionContext;

// Neural network activation functions
enum class ActivationFunction : uint8
{
    LINEAR,
    SIGMOID,
    TANH,
    RELU,
    LEAKY_RELU,
    SOFTMAX
};

// Learning algorithm types
enum class LearningAlgorithm : uint8
{
    Q_LEARNING,
    DEEP_Q_NETWORK,
    POLICY_GRADIENT,
    ACTOR_CRITIC,
    EVOLUTIONARY,
    IMITATION
};

// Experience replay sample
struct Experience
{
    ::std::vector<float> state;           // Current state features
    uint32_t action;                     // Action taken
    float reward;                        // Immediate reward
    ::std::vector<float> nextState;       // Resulting state
    bool terminal;                       // Episode ended?
    uint64_t timestamp;                  // When this experience occurred
    float importance;                    // Importance sampling weight

    Experience() : action(0), reward(0.0f), terminal(false), timestamp(0), importance(1.0f) {}
};

// Neural network layer
struct NeuralLayer
{
    ::std::vector<::std::vector<float>> weights;  // Weight matrix
    ::std::vector<float> biases;                // Bias vector
    ::std::vector<float> outputs;               // Layer outputs
    ::std::vector<float> gradients;             // Backpropagation gradients
    ActivationFunction activation;            // Activation function

    void Forward(const ::std::vector<float>& input);
    void Backward(const ::std::vector<float>& error, float learningRate);
    void Initialize(size_t inputSize, size_t outputSize, ActivationFunction actFunc);
    void UpdateWeights(float learningRate, float momentum = 0.9f);
};

// Simple neural network implementation
class TC_GAME_API NeuralNetwork
{
public:
    NeuralNetwork();
    ~NeuralNetwork();

    // Network construction
    void AddLayer(size_t neurons, ActivationFunction activation = ActivationFunction::RELU);
    void Build(size_t inputSize);

    // Forward propagation
    ::std::vector<float> Predict(const ::std::vector<float>& input);

    // Training
    void Train(const ::std::vector<float>& input, const ::std::vector<float>& target, float learningRate);
    void BatchTrain(const ::std::vector<Experience>& batch, float learningRate);

    // Network management
    void SaveWeights(const ::std::string& filename) const;
    void LoadWeights(const ::std::string& filename);
    void CopyFrom(const NeuralNetwork& other);
    void Reset();

    // Metrics
    float GetLoss() const { return _lastLoss; }
    uint32_t GetEpoch() const { return _epoch; }

private:
    ::std::vector<NeuralLayer> _layers;
    size_t _inputSize;
    float _lastLoss;
    uint32_t _epoch;
    ::std::mt19937 _rng;

    // Activation functions
    float Activate(float x, ActivationFunction func) const;
    float ActivateDerivative(float x, ActivationFunction func) const;
    ::std::vector<float> Softmax(const ::std::vector<float>& input) const;
};

// Q-Learning value function approximator
class TC_GAME_API QFunction
{
public:
    QFunction(size_t stateSize, size_t actionSize);
    ~QFunction();

    // Q-value operations
    float GetQValue(const ::std::vector<float>& state, uint32_t action) const;
    ::std::vector<float> GetAllQValues(const ::std::vector<float>& state) const;
    uint32_t GetBestAction(const ::std::vector<float>& state) const;

    // Learning
    void Update(const Experience& exp, float learningRate, float discountFactor);
    void BatchUpdate(const ::std::vector<Experience>& batch, float learningRate, float discountFactor);

    // Exploration
    uint32_t SelectAction(const ::std::vector<float>& state, float epsilon);

private:
    ::std::unique_ptr<NeuralNetwork> _network;
    ::std::unique_ptr<NeuralNetwork> _targetNetwork;  // For stable learning
    size_t _stateSize;
    size_t _actionSize;
    uint32_t _updateCounter;
    ::std::mt19937 _rng;

    static constexpr uint32_t TARGET_UPDATE_FREQUENCY = 100;
};

// Policy gradient network for direct policy learning
class TC_GAME_API PolicyNetwork
{
public:
    PolicyNetwork(size_t stateSize, size_t actionSize);
    ~PolicyNetwork();

    // Policy operations
    ::std::vector<float> GetActionProbabilities(const ::std::vector<float>& state);
    uint32_t SampleAction(const ::std::vector<float>& state);

    // Training
    void UpdatePolicy(const ::std::vector<Experience>& trajectory, float learningRate);
    void ComputeAdvantage(::std::vector<Experience>& trajectory, float gamma);

private:
    ::std::unique_ptr<NeuralNetwork> _policyNet;
    ::std::unique_ptr<NeuralNetwork> _valueNet;  // For advantage estimation
    size_t _stateSize;
    size_t _actionSize;
    ::std::mt19937 _rng;
};

// Main behavior adaptation engine
class TC_GAME_API BehaviorAdaptation
{
public:
    static BehaviorAdaptation& Instance()
    {
        static BehaviorAdaptation instance;
        return instance;
    }

    // System initialization
    bool Initialize();
    void Shutdown();

    // Learning configuration
    void SetLearningAlgorithm(LearningAlgorithm algo) { _algorithm = algo; }
    void SetLearningRate(float rate) { _learningRate = ::std::clamp(rate, 0.0001f, 0.1f); }
    void SetDiscountFactor(float factor) { _discountFactor = ::std::clamp(factor, 0.0f, 0.99f); }
    void SetExplorationRate(float rate) { _epsilon = ::std::clamp(rate, 0.0f, 1.0f); }
    void EnableLearning(bool enable) { _learningEnabled = enable; }

    // State feature extraction
    ::std::vector<float> ExtractStateFeatures(BotAI* ai, Player* bot) const;
    ::std::vector<float> ExtractCombatFeatures(Player* bot, Unit* target) const;
    ::std::vector<float> ExtractSocialFeatures(Player* bot) const;
    ::std::vector<float> ExtractEnvironmentFeatures(Player* bot) const;

    // Action mapping
    uint32_t MapActionToIndex(const ::std::string& actionName) const;
    ::std::string MapIndexToAction(uint32_t index) const;
    void RegisterAction(const ::std::string& actionName);

    // Reward calculation
    float CalculateReward(BotAI* ai, const ActionContext& context, bool success) const;
    float CalculateCombatReward(Player* bot, float damageDealt, float damageTaken) const;
    float CalculateSocialReward(Player* bot, bool interactionSuccess) const;
    float CalculateQuestReward(Player* bot, bool questProgress) const;

    // Learning operations
    void RecordExperience(uint32_t botGuid, const Experience& exp);
    void Learn(uint32_t botGuid);
    void BatchLearn(uint32_t botGuid, size_t batchSize = 32);

    // Decision making
    uint32_t SelectAction(uint32_t botGuid, const ::std::vector<float>& state);
    float EvaluateAction(uint32_t botGuid, const ::std::vector<float>& state, uint32_t action);

    // Model management
    void SaveModel(uint32_t botGuid, const ::std::string& filename);
    void LoadModel(uint32_t botGuid, const ::std::string& filename);
    void ShareKnowledge(uint32_t sourceBotGuid, uint32_t targetBotGuid);

    // Performance tracking
    struct LearningMetrics
    {
        ::std::atomic<uint64_t> totalExperiences{0};
        ::std::atomic<uint64_t> learningSteps{0};
        ::std::atomic<float> averageReward{0.0f};
        ::std::atomic<float> averageLoss{0.0f};
        ::std::atomic<float> winRate{0.0f};
        ::std::chrono::steady_clock::time_point startTime;

        void Reset()
        {
            totalExperiences = 0;
            learningSteps = 0;
            averageReward = 0.0f;
            averageLoss = 0.0f;
            winRate = 0.0f;
            startTime = ::std::chrono::steady_clock::now();
        }
    };

    LearningMetrics GetMetrics(uint32_t botGuid) const;
    void ResetMetrics(uint32_t botGuid);

    // Collective intelligence
    void UpdateCollectiveKnowledge();
    void PropagateSuccessfulStrategies();
    ::std::vector<float> GetCollectivePolicy(const ::std::vector<float>& state);

    // Meta-learning
    void AdaptToGameMeta();
    void IdentifyMetaShift();
    void UpdateMetaStrategies();

    // Exploration strategies
    void SetExplorationStrategy(const ::std::string& strategy);
    float GetAdaptiveEpsilon(uint32_t botGuid) const;
    void UpdateExplorationRate(uint32_t botGuid);

private:
    BehaviorAdaptation();
    ~BehaviorAdaptation();

    // Bot-specific learning models
    struct BotLearningModel
    {
        ::std::unique_ptr<QFunction> qFunction;
        ::std::unique_ptr<PolicyNetwork> policyNetwork;
        ::std::deque<Experience> experienceBuffer;
        LearningMetrics metrics;
        float epsilon;  // Exploration rate
        uint32_t episodeCount;
        uint32_t stepCount;

        static constexpr size_t MAX_BUFFER_SIZE = 10000;
    };

    // Learning system state
    bool _initialized;
    bool _learningEnabled;
    LearningAlgorithm _algorithm;
    float _learningRate;
    float _discountFactor;
    float _epsilon;  // Global exploration rate
    float _epsilonDecay;
    float _epsilonMin;

    // Model storage
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE> _modelsMutex;
    ::std::unordered_map<uint32_t, ::std::unique_ptr<BotLearningModel>> _botModels;

    // Action registry
    ::std::vector<::std::string> _actionRegistry;
    ::std::unordered_map<::std::string, uint32_t> _actionToIndex;

    // Collective intelligence
    ::std::unique_ptr<NeuralNetwork> _collectiveModel;
    ::std::deque<Experience> _sharedExperiences;
    static constexpr size_t MAX_SHARED_EXPERIENCES = 50000;

    // Meta-learning
    struct MetaStrategy
    {
        ::std::string name;
        ::std::vector<float> features;
        float successRate;
        uint32_t usageCount;
        ::std::chrono::steady_clock::time_point lastUsed;
    };
    ::std::vector<MetaStrategy> _metaStrategies;

    // Helper methods
    BotLearningModel* GetOrCreateModel(uint32_t botGuid);
    void InitializeModel(BotLearningModel* model);
    void CleanupOldExperiences(BotLearningModel* model);
    ::std::vector<Experience> SampleBatch(const ::std::deque<Experience>& buffer, size_t batchSize);
    float CalculatePriority(const Experience& exp) const;
    void NormalizeFeatures(::std::vector<float>& features) const;

    // State feature dimensions
    static constexpr size_t STATE_SIZE = 128;
    static constexpr size_t ACTION_SIZE = 64;
    static constexpr size_t HIDDEN_SIZE = 256;

    // Learning parameters
    static constexpr float DEFAULT_LEARNING_RATE = 0.001f;
    static constexpr float DEFAULT_DISCOUNT_FACTOR = 0.95f;
    static constexpr float DEFAULT_EPSILON = 0.9f;
    static constexpr float EPSILON_DECAY_RATE = 0.995f;
    static constexpr float MIN_EPSILON = 0.1f;

    // Performance thresholds
    static constexpr float MIN_REWARD_THRESHOLD = -10.0f;
    static constexpr float MAX_REWARD_THRESHOLD = 10.0f;
    static constexpr uint32_t MIN_EXPERIENCES_FOR_LEARNING = 100;
    static constexpr uint32_t COLLECTIVE_UPDATE_INTERVAL = 1000;
};

// RAII helper for scoped learning sessions
class TC_GAME_API ScopedLearningSession
{
public:
    ScopedLearningSession(uint32_t botGuid, BotAI* ai);
    ~ScopedLearningSession();

    void RecordAction(const ::std::string& action, bool success);
    void RecordReward(float reward);
    void Commit();

private:
    uint32_t _botGuid;
    BotAI* _ai;
    ::std::vector<float> _initialState;
    ::std::string _action;
    float _cumulativeReward;
    bool _committed;
};

#define sBehaviorAdaptation BehaviorAdaptation::Instance()

} // namespace Playerbot