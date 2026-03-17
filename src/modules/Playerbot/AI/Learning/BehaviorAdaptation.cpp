/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "BehaviorAdaptation.h"
#include "SpellHistory.h"
#include "AI/BotAI.h"
#include "Player.h"
#include "Unit.h"
#include "Group.h"
#include "Log.h"
#include "World.h"
#include "Performance/BotPerformanceMonitor.h"
#include "../../Core/Threading/SafeGridOperations.h"  // SEH-protected grid operations
#include <fstream>
#include <sstream>

namespace Playerbot
{

// Neural Layer Implementation
void NeuralLayer::Forward(const ::std::vector<float>& input)
{
    outputs.resize(weights.size());

    for (size_t i = 0; i < weights.size(); ++i)
    {
        float sum = biases[i];
        for (size_t j = 0; j < input.size(); ++j)
        {
            sum += input[j] * weights[i][j];
        }

        // Apply activation function
    switch (activation)
        {
            case ActivationFunction::LINEAR:
                outputs[i] = sum;
                break;
            case ActivationFunction::SIGMOID:
                outputs[i] = 1.0f / (1.0f + ::std::exp(-sum));
                break;
            case ActivationFunction::TANH:
                outputs[i] = ::std::tanh(sum);
                break;
            case ActivationFunction::RELU:
                outputs[i] = ::std::max(0.0f, sum);
                break;
            case ActivationFunction::LEAKY_RELU:
                outputs[i] = sum > 0 ? sum : 0.01f * sum;
                break;
            default:
                outputs[i] = sum;
                break;
        }
    }
}

void NeuralLayer::Initialize(size_t inputSize, size_t outputSize, ActivationFunction actFunc)
{
    activation = actFunc;
    weights.resize(outputSize);
    biases.resize(outputSize);
    outputs.resize(outputSize);
    gradients.resize(outputSize);

    // Xavier initialization
    float limit = ::std::sqrt(6.0f / (inputSize + outputSize));
    ::std::random_device rd;
    ::std::mt19937 gen(rd());
    ::std::uniform_real_distribution<float> dist(-limit, limit);

    for (size_t i = 0; i < outputSize; ++i)
    {
        weights[i].resize(inputSize);
        for (size_t j = 0; j < inputSize; ++j)
        {
            weights[i][j] = dist(gen);
        }
        biases[i] = 0.0f;
    }
}

// Neural Network Implementation
NeuralNetwork::NeuralNetwork() : _inputSize(0), _lastLoss(0.0f), _epoch(0)
{
    ::std::random_device rd;
    _rng.seed(rd());
}

NeuralNetwork::~NeuralNetwork() = default;

void NeuralNetwork::AddLayer(size_t neurons, ActivationFunction activation)
{
    NeuralLayer layer;
    layer.activation = activation;
    layer.outputs.resize(neurons);
    layer.gradients.resize(neurons);
    _layers.push_back(::std::move(layer));
}

void NeuralNetwork::Build(size_t inputSize)
{
    _inputSize = inputSize;

    if (_layers.empty())
        return;

    // Initialize first layer
    _layers[0].Initialize(inputSize, _layers[0].outputs.size(), _layers[0].activation);

    // Initialize subsequent layers
    for (size_t i = 1; i < _layers.size(); ++i)
    {
        size_t prevSize = _layers[i-1].outputs.size();
        _layers[i].Initialize(prevSize, _layers[i].outputs.size(), _layers[i].activation);
    }
}

::std::vector<float> NeuralNetwork::Predict(const ::std::vector<float>& input)
{
    if (_layers.empty() || input.size() != _inputSize)
        return ::std::vector<float>();

    ::std::vector<float> currentInput = input;

    for (auto& layer : _layers)
    {
        layer.Forward(currentInput);
        currentInput = layer.outputs;
    }

    return _layers.back().outputs;
}

void NeuralNetwork::Train(const ::std::vector<float>& input, const ::std::vector<float>& target, float learningRate)
{
    // Forward pass
    ::std::vector<float> prediction = Predict(input);

    if (prediction.size() != target.size())
        return;

    // Calculate loss (MSE)
    _lastLoss = 0.0f;
    ::std::vector<float> error(target.size());
    for (size_t i = 0; i < target.size(); ++i)
    {
        error[i] = target[i] - prediction[i];
        _lastLoss += error[i] * error[i];
    }
    _lastLoss /= target.size();

    // Backpropagation
    for (int i = static_cast<int>(_layers.size()) - 1; i >= 0; --i)
    {
        auto& layer = _layers[static_cast<size_t>(i)];

        if (static_cast<size_t>(i) == _layers.size() - 1)
        {
            // Output layer
            layer.gradients = error;
        }
        else
        {
            // Hidden layers
            ::std::vector<float> nextError(layer.outputs.size(), 0.0f);
            auto& nextLayer = _layers[static_cast<size_t>(i + 1)];

            for (size_t j = 0; j < layer.outputs.size(); ++j)
            {
                for (size_t k = 0; k < nextLayer.weights.size(); ++k)
                {
                    nextError[j] += nextLayer.gradients[k] * nextLayer.weights[k][j];
                }

                // Apply activation derivative
                nextError[j] *= ActivateDerivative(layer.outputs[j], layer.activation);
            }
            layer.gradients = nextError;
        }

        // Update weights and biases
        const ::std::vector<float>& layerInput = (i == 0) ? input : _layers[i-1].outputs;

        for (size_t j = 0; j < layer.weights.size(); ++j)
        {
            for (size_t k = 0; k < layer.weights[j].size(); ++k)
            {
                layer.weights[j][k] += learningRate * layer.gradients[j] * layerInput[k];
            }
            layer.biases[j] += learningRate * layer.gradients[j];
        }
    }

    _epoch++;
}

float NeuralNetwork::ActivateDerivative(float x, ActivationFunction func) const
{
    switch (func)
    {
        case ActivationFunction::LINEAR:
            return 1.0f;
        case ActivationFunction::SIGMOID:
            return x * (1.0f - x);
        case ActivationFunction::TANH:
            return 1.0f - x * x;
        case ActivationFunction::RELU:
            return x > 0 ? 1.0f : 0.0f;
        case ActivationFunction::LEAKY_RELU:
            return x > 0 ? 1.0f : 0.01f;
        default:
            return 1.0f;
    }
}

// Q-Function Implementation
QFunction::QFunction(size_t stateSize, size_t actionSize)
    : _stateSize(stateSize), _actionSize(actionSize), _updateCounter(0)
{
    ::std::random_device rd;
    _rng.seed(rd());

    // Build Q-network
    _network = ::std::make_unique<NeuralNetwork>();
    _network->AddLayer(128, ActivationFunction::RELU);
    _network->AddLayer(128, ActivationFunction::RELU);
    _network->AddLayer(actionSize, ActivationFunction::LINEAR);
    _network->Build(stateSize);

    // Build target network (for stability)
    _targetNetwork = ::std::make_unique<NeuralNetwork>();
    _targetNetwork->AddLayer(128, ActivationFunction::RELU);
    _targetNetwork->AddLayer(128, ActivationFunction::RELU);
    _targetNetwork->AddLayer(actionSize, ActivationFunction::LINEAR);
    _targetNetwork->Build(stateSize);
}

QFunction::~QFunction() = default;

float QFunction::GetQValue(const ::std::vector<float>& state, uint32_t action) const
{
    if (action >= _actionSize)
        return 0.0f;

    ::std::vector<float> qValues = _network->Predict(state);
    return qValues.empty() ? 0.0f : qValues[action];
}

::std::vector<float> QFunction::GetAllQValues(const ::std::vector<float>& state) const
{
    return _network->Predict(state);
}

uint32_t QFunction::GetBestAction(const ::std::vector<float>& state) const
{
    ::std::vector<float> qValues = GetAllQValues(state);
    if (qValues.empty())
        return 0;

    auto maxIt = ::std::max_element(qValues.begin(), qValues.end());
    return ::std::distance(qValues.begin(), maxIt);
}

void QFunction::Update(const Experience& exp, float learningRate, float discountFactor)
{
    // Calculate target Q-value
    float targetQ = exp.reward;
    if (!exp.terminal)
    {
        ::std::vector<float> nextQValues = _targetNetwork->Predict(exp.nextState);
        if (!nextQValues.empty())
        {
            float maxNextQ = *::std::max_element(nextQValues.begin(), nextQValues.end());
            targetQ += discountFactor * maxNextQ;
        }
    }

    // Get current Q-values and update target for selected action
    ::std::vector<float> currentQValues = _network->Predict(exp.state);
    if (!currentQValues.empty() && exp.action < currentQValues.size())
    {
        currentQValues[exp.action] = targetQ;
        _network->Train(exp.state, currentQValues, learningRate);
    }

    // Update target network periodically
    _updateCounter++;
    if (_updateCounter % TARGET_UPDATE_FREQUENCY == 0)
    {
        _targetNetwork->CopyFrom(*_network);
    }
}

uint32_t QFunction::SelectAction(const ::std::vector<float>& state, float epsilon)
{
    ::std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    if (dist(_rng) < epsilon)
    {
        // Exploration: random action
        ::std::uniform_int_distribution<uint32_t> actionDist(0, _actionSize - 1);
        return actionDist(_rng);
    }
    else
    {
        // Exploitation: best action
        return GetBestAction(state);
    }
}

// Policy Network Implementation
PolicyNetwork::PolicyNetwork(size_t stateSize, size_t actionSize)
    : _stateSize(stateSize), _actionSize(actionSize)
{
    ::std::random_device rd;
    _rng.seed(rd());

    // Build policy network
    _policyNet = ::std::make_unique<NeuralNetwork>();
    _policyNet->AddLayer(128, ActivationFunction::RELU);
    _policyNet->AddLayer(64, ActivationFunction::RELU);
    _policyNet->AddLayer(actionSize, ActivationFunction::SOFTMAX);
    _policyNet->Build(stateSize);

    // Build value network for advantage estimation
    _valueNet = ::std::make_unique<NeuralNetwork>();
    _valueNet->AddLayer(128, ActivationFunction::RELU);
    _valueNet->AddLayer(64, ActivationFunction::RELU);
    _valueNet->AddLayer(1, ActivationFunction::LINEAR);
    _valueNet->Build(stateSize);
}

PolicyNetwork::~PolicyNetwork() = default;

::std::vector<float> PolicyNetwork::GetActionProbabilities(const ::std::vector<float>& state)
{
    return _policyNet->Predict(state);
}

uint32_t PolicyNetwork::SampleAction(const ::std::vector<float>& state)
{
    ::std::vector<float> probs = GetActionProbabilities(state);
    if (probs.empty())
        return 0;

    // Sample from probability distribution
    ::std::discrete_distribution<uint32_t> dist(probs.begin(), probs.end());
    return dist(_rng);
}

// BehaviorAdaptation Implementation
BehaviorAdaptation::BehaviorAdaptation()
    : _initialized(false)
    , _learningEnabled(true)
    , _algorithm(LearningAlgorithm::Q_LEARNING)
    , _learningRate(DEFAULT_LEARNING_RATE)
    , _discountFactor(DEFAULT_DISCOUNT_FACTOR)
    , _epsilon(DEFAULT_EPSILON)
    , _epsilonDecay(EPSILON_DECAY_RATE)
    , _epsilonMin(MIN_EPSILON)
{
}

BehaviorAdaptation::~BehaviorAdaptation()
{
    Shutdown();
}

bool BehaviorAdaptation::Initialize()
{
    if (_initialized)
        return true;

    TC_LOG_INFO("playerbot.learning", "Initializing Behavior Adaptation System");

    // Initialize action registry with common actions
    RegisterAction("attack");
    RegisterAction("cast_spell");
    RegisterAction("move_to_target");
    RegisterAction("retreat");
    RegisterAction("use_consumable");
    RegisterAction("assist_ally");
    RegisterAction("crowd_control");
    RegisterAction("interrupt");
    RegisterAction("defensive_stance");
    RegisterAction("offensive_stance");

    // Initialize collective model
    _collectiveModel = ::std::make_unique<NeuralNetwork>();
    _collectiveModel->AddLayer(256, ActivationFunction::RELU);
    _collectiveModel->AddLayer(128, ActivationFunction::RELU);
    _collectiveModel->AddLayer(ACTION_SIZE, ActivationFunction::SOFTMAX);
    _collectiveModel->Build(STATE_SIZE);

    _initialized = true;
    TC_LOG_INFO("playerbot.learning", "Behavior Adaptation System initialized successfully");
    return true;
}

void BehaviorAdaptation::Shutdown()
{
    if (!_initialized)
        return;

    TC_LOG_INFO("playerbot.learning", "Shutting down Behavior Adaptation System");

    ::std::lock_guard lock(_modelsMutex);
    _botModels.clear();
    _collectiveModel.reset();
    _sharedExperiences.clear();
    _metaStrategies.clear();

    _initialized = false;
}

::std::vector<float> BehaviorAdaptation::ExtractStateFeatures(BotAI* ai, Player* bot) const
{
    ::std::vector<float> features;
    features.reserve(STATE_SIZE);

    if (!ai || !bot)
    {
        features.resize(STATE_SIZE, 0.0f);
        return features;
    }

    // Health and resource features
    features.push_back(bot->GetHealthPct() / 100.0f);
    features.push_back(bot->GetPowerPct(bot->GetPowerType()) / 100.0f);

    // Position features
    Position pos = bot->GetPosition();
    features.push_back(pos.GetPositionX() / 10000.0f);  // Normalized
    features.push_back(pos.GetPositionY() / 10000.0f);
    features.push_back(pos.GetPositionZ() / 1000.0f);

    // Combat state
    features.push_back(bot->IsInCombat() ? 1.0f : 0.0f);
    features.push_back(bot->GetVictim() ? 1.0f : 0.0f);

    // AI state features
    features.push_back(static_cast<float>(ai->GetAIState()) / 10.0f);
    // Group features
    if (Group* group = bot->GetGroup())
    {
        features.push_back(group->GetMembersCount() / 5.0f);
        features.push_back(group->IsRaidGroup() ? 1.0f : 0.0f);
    }
    else
    {
        features.push_back(0.0f);
        features.push_back(0.0f);
    }

    // Target features
    if (Unit* target = ai->GetTargetUnit())
    {
        features.push_back(target->GetHealthPct() / 100.0f);
        features.push_back(::std::sqrt(bot->GetExactDistSq(target)) / 50.0f); // Calculate once from squared distance
        features.push_back(target->GetLevel() / 80.0f);
        features.push_back(target->IsPlayer() ? 1.0f : 0.0f);
    }
    else
    {
        features.push_back(0.0f);
        features.push_back(0.0f);
        features.push_back(0.0f);
        features.push_back(0.0f);
    }

    // Add combat features
    ::std::vector<float> combatFeatures = ExtractCombatFeatures(bot, ai->GetTargetUnit());
    features.insert(features.end(), combatFeatures.begin(), combatFeatures.end());

    // Add social features
    ::std::vector<float> socialFeatures = ExtractSocialFeatures(bot);
    features.insert(features.end(), socialFeatures.begin(), socialFeatures.end());

    // Add environment features
    ::std::vector<float> envFeatures = ExtractEnvironmentFeatures(bot);
    features.insert(features.end(), envFeatures.begin(), envFeatures.end());

    // Pad or truncate to STATE_SIZE
    features.resize(STATE_SIZE, 0.0f);

    // Normalize features
    NormalizeFeatures(features);

    return features;
}

::std::vector<float> BehaviorAdaptation::ExtractCombatFeatures(Player* bot, Unit* target) const
{
    ::std::vector<float> features;

    if (!bot)
    {
        features.resize(20, 0.0f);
        return features;
    }

    // Offensive stats
    features.push_back(bot->GetTotalAttackPowerValue(BASE_ATTACK) / 5000.0f);
    features.push_back(bot->GetFloatValue(PLAYER_FIELD_CRIT_PERCENTAGE) / 100.0f);
    features.push_back(bot->GetFloatValue(PLAYER_FIELD_HASTE_RATING) / 100.0f);

    // Defensive stats
    features.push_back(bot->GetArmor() / 20000.0f);
    features.push_back(bot->GetFloatValue(PLAYER_FIELD_DODGE_PERCENTAGE) / 100.0f);
    features.push_back(bot->GetFloatValue(PLAYER_FIELD_PARRY_PERCENTAGE) / 100.0f);

    // Cooldown availability (simplified)
    features.push_back(bot->GetSpellHistory()->HasCooldown(61304) ? 0.0f : 1.0f);  // Global cooldown check

    // Buff/debuff counts
    uint32_t buffCount = 0, debuffCount = 0;
    Unit::AuraApplicationMap const& auras = bot->GetAppliedAuras();
    for (auto const& [auraId, auraApp] : auras)
    {
        if (auraApp->GetBase()->IsPositive())
            buffCount++;
        else
            debuffCount++;
    }
    features.push_back(buffCount / 10.0f);
    features.push_back(debuffCount / 5.0f);

    // Target-specific features
    if (target)
    {
        features.push_back(target->GetCreatureType() / 20.0f);
        features.push_back(target->GetHealthPct() / 100.0f);
        features.push_back(bot->IsWithinMeleeRange(target) ? 1.0f : 0.0f);
        features.push_back(bot->IsWithinLOSInMap(target) ? 1.0f : 0.0f);

        // Threat level (simplified)
        features.push_back(target->GetThreatManager().GetThreat(bot) / 10000.0f);
    }
    else
    {
        features.push_back(0.0f);
        features.push_back(0.0f);
        features.push_back(0.0f);
        features.push_back(0.0f);
        features.push_back(0.0f);
    }

    // Pad to fixed size
    while (features.size() < 20)
        features.push_back(0.0f);

    return features;
}

::std::vector<float> BehaviorAdaptation::ExtractSocialFeatures(Player* bot) const
{
    ::std::vector<float> features;

    if (!bot)
    {
        features.resize(10, 0.0f);
        return features;
    }

    // Group dynamics
    if (Group* group = bot->GetGroup())
    {
        features.push_back(1.0f);  // In group
        features.push_back(group->GetMembersCount() / 40.0f);  // Group size
        features.push_back(group->GetLeaderGuid() == bot->GetGUID() ? 1.0f : 0.0f);  // Is leader
        // Average group health
        float avgHealth = 0.0f;
        uint32_t memberCount = 0;
        for (GroupReference* itr : *group)
        {
            if (Player* member = itr->GetSource())
            {
                avgHealth += member->GetHealthPct();
                memberCount++;
            }
        }
        features.push_back(memberCount > 0 ? (avgHealth / memberCount / 100.0f) : 0.0f);
    }
    else
    {
        features.push_back(0.0f);  // Not in group
        features.push_back(0.0f);
        features.push_back(0.0f);
        features.push_back(0.0f);
    }

    // Guild membership
    features.push_back(bot->GetGuildId() ? 1.0f : 0.0f);

    // Social interactions (simplified)
    features.push_back(0.0f);  // Trade activity
    features.push_back(0.0f);  // Chat activity
    features.push_back(0.0f);  // Emote usage

    // Friend/ignore list sizes (normalized)
    features.push_back(0.0f);  // Friends count / 50
    features.push_back(0.0f);  // Ignores count / 20

    return features;
}

::std::vector<float> BehaviorAdaptation::ExtractEnvironmentFeatures(Player* bot) const
{
    ::std::vector<float> features;

    if (!bot)
    {
        features.resize(15, 0.0f);
        return features;
    }

    // Zone and area
    features.push_back(bot->GetZoneId() / 10000.0f);
    features.push_back(bot->GetAreaId() / 10000.0f);

    // Map type
    Map* map = bot->GetMap();
    if (map)
    {
        features.push_back(map->IsDungeon() ? 1.0f : 0.0f);
        features.push_back(map->IsRaid() ? 1.0f : 0.0f);
        features.push_back(map->IsBattleground() ? 1.0f : 0.0f);
        features.push_back(map->IsArena() ? 1.0f : 0.0f);
    }
    else
    {
        features.push_back(0.0f);
        features.push_back(0.0f);
        features.push_back(0.0f);
        features.push_back(0.0f);
    }

    // Time of day (in-game)
    features.push_back(0.5f);  // Normalized game time

    // Weather (simplified)
    features.push_back(0.0f);  // Weather intensity

    // Nearby entity counts
    // THREAD-SAFE: Use SafeGridOperations with SEH protection to catch access violations
    ::std::list<Player*> nearbyPlayers;
    ::std::list<Creature*> nearbyCreatures;
    SafeGridOperations::GetPlayerListSafe(bot, nearbyPlayers, 30.0f);
    SafeGridOperations::GetCreatureListSafe(bot, nearbyCreatures, 0, 30.0f);

    features.push_back(nearbyPlayers.size() / 20.0f);
    features.push_back(nearbyCreatures.size() / 30.0f);
    // Movement state
    features.push_back(bot->isMoving() ? 1.0f : 0.0f);
    features.push_back(bot->IsFalling() ? 1.0f : 0.0f);
    features.push_back(bot->IsFlying() ? 1.0f : 0.0f);
    // Pad to fixed size
    while (features.size() < 15)
        features.push_back(0.0f);

    return features;
}

void BehaviorAdaptation::RegisterAction(const ::std::string& actionName)
{
    if (_actionToIndex.find(actionName) == _actionToIndex.end())
    {
        uint32_t index = _actionRegistry.size();
        _actionRegistry.push_back(actionName);
        _actionToIndex[actionName] = index;
    }
}

uint32_t BehaviorAdaptation::MapActionToIndex(const ::std::string& actionName) const
{
    auto it = _actionToIndex.find(actionName);
    return it != _actionToIndex.end() ? it->second : 0;
}

::std::string BehaviorAdaptation::MapIndexToAction(uint32_t index) const
{
    if (index < _actionRegistry.size())
        return _actionRegistry[index];
    return "unknown";
}

float BehaviorAdaptation::CalculateReward(BotAI* ai, const ActionContext& context, bool success) const
{
    float reward = 0.0f;

    // Base reward for action success/failure
    reward += success ? 1.0f : -0.5f;

    // Additional context-based rewards
    if (ai && ai->GetBot())
    {
        Player* bot = ai->GetBot();

        // Health preservation reward
        float healthPct = bot->GetHealthPct();
        if (healthPct > 80.0f)
            reward += 0.2f;
        else if (healthPct < 30.0f)
            reward -= 0.3f;

        // Combat effectiveness
    if (bot->IsInCombat())
        {
            if (context.damageDealt > 0)
                reward += ::std::min(context.damageDealt / 10000.0f, 1.0f);
            if (context.damageTaken > 0)
                reward -= ::std::min(context.damageTaken / 5000.0f, 0.5f);
        }

        // Group cooperation
    if (bot->GetGroup() && context.helpedAlly)
            reward += 0.5f;
    }

    // Clamp reward
    return ::std::clamp(reward, MIN_REWARD_THRESHOLD, MAX_REWARD_THRESHOLD);
}

void BehaviorAdaptation::RecordExperience(uint32_t botGuid, const Experience& exp)
{
    if (!_learningEnabled)
        return;

    MEASURE_PERFORMANCE(MetricType::AI_DECISION_TIME, botGuid, "RecordExperience");

    ::std::lock_guard lock(_modelsMutex);

    BotLearningModel* model = GetOrCreateModel(botGuid);
    if (!model)
        return;

    // Add to bot's experience buffer
    model->experienceBuffer.push_back(exp);
    if (model->experienceBuffer.size() > BotLearningModel::MAX_BUFFER_SIZE)
    {
        model->experienceBuffer.pop_front();
    }

    // Add to shared experiences for collective learning
    if (exp.reward > 0.5f)  // Only share successful experiences
    {
        _sharedExperiences.push_back(exp);
        if (_sharedExperiences.size() > MAX_SHARED_EXPERIENCES)
        {
            _sharedExperiences.pop_front();
        }
    }

    // Update metrics
    model->metrics.totalExperiences++;

    // Update running average reward
    float alpha = 0.01f;  // Learning rate for running average
    model->metrics.averageReward = (1.0f - alpha) * model->metrics.averageReward + alpha * exp.reward;
}

void BehaviorAdaptation::Learn(uint32_t botGuid)
{
    if (!_learningEnabled)
        return;

    ::std::lock_guard lock(_modelsMutex);

    BotLearningModel* model = GetOrCreateModel(botGuid);
    if (!model || model->experienceBuffer.size() < MIN_EXPERIENCES_FOR_LEARNING)
        return;

    MEASURE_PERFORMANCE(MetricType::AI_DECISION_TIME, botGuid, "Learning");

    // Select learning method based on algorithm
    switch (_algorithm)
    {
        case LearningAlgorithm::Q_LEARNING:
        case LearningAlgorithm::DEEP_Q_NETWORK:
        {
            if (model->qFunction)
            {
                // Sample recent experience
                const Experience& exp = model->experienceBuffer.back();
                model->qFunction->Update(exp, _learningRate, _discountFactor);
            }
            break;
        }
        case LearningAlgorithm::POLICY_GRADIENT:
        case LearningAlgorithm::ACTOR_CRITIC:
        {
            if (model->policyNetwork)
            {
                // Collect trajectory
                ::std::vector<Experience> trajectory(
                    model->experienceBuffer.end() - ::std::min<size_t>(32, model->experienceBuffer.size()),
                    model->experienceBuffer.end());
                model->policyNetwork->UpdatePolicy(trajectory, _learningRate);
            }
            break;
        }
        default:
            break;
    }

    model->metrics.learningSteps++;
    model->stepCount++;

    // Update exploration rate
    UpdateExplorationRate(botGuid);
}

void BehaviorAdaptation::BatchLearn(uint32_t botGuid, size_t batchSize)
{
    if (!_learningEnabled)
        return;

    ::std::lock_guard lock(_modelsMutex);

    BotLearningModel* model = GetOrCreateModel(botGuid);
    if (!model || model->experienceBuffer.size() < batchSize)
        return;

    MEASURE_PERFORMANCE(MetricType::AI_DECISION_TIME, botGuid, "BatchLearning");

    // Sample batch from experience buffer
    ::std::vector<Experience> batch = SampleBatch(model->experienceBuffer, batchSize);

    // Batch update based on algorithm
    if (model->qFunction && (_algorithm == LearningAlgorithm::Q_LEARNING ||
                             _algorithm == LearningAlgorithm::DEEP_Q_NETWORK))
    {
        model->qFunction->BatchUpdate(batch, _learningRate, _discountFactor);
    }

    model->metrics.learningSteps += batch.size();
}

uint32_t BehaviorAdaptation::SelectAction(uint32_t botGuid, const ::std::vector<float>& state)
{
    ::std::lock_guard lock(_modelsMutex);

    BotLearningModel* model = GetOrCreateModel(botGuid);
    if (!model)
        return 0;

    uint32_t action = 0;

    switch (_algorithm)
    {
        case LearningAlgorithm::Q_LEARNING:
        case LearningAlgorithm::DEEP_Q_NETWORK:
        {
            if (model->qFunction)
            {
                float epsilon = GetAdaptiveEpsilon(botGuid);
                action = model->qFunction->SelectAction(state, epsilon);
            }
            break;
        }
        case LearningAlgorithm::POLICY_GRADIENT:
        case LearningAlgorithm::ACTOR_CRITIC:
        {
            if (model->policyNetwork)
            {
                action = model->policyNetwork->SampleAction(state);
            }
            break;
        }
        default:
            // Random action as fallback
            ::std::uniform_int_distribution<uint32_t> dist(0, ACTION_SIZE - 1);
            ::std::mt19937 rng(::std::chrono::steady_clock::now().time_since_epoch().count());
            action = dist(rng);
            break;
    }

    return action;
}

float BehaviorAdaptation::GetAdaptiveEpsilon(uint32_t botGuid) const
{
    ::std::lock_guard lock(_modelsMutex);

    auto it = _botModels.find(botGuid);
    if (it != _botModels.end())
    {
        return it->second->epsilon;
    }
    return _epsilon;
}

void BehaviorAdaptation::UpdateExplorationRate(uint32_t botGuid)
{
    ::std::lock_guard lock(_modelsMutex);

    auto it = _botModels.find(botGuid);
    if (it != _botModels.end())
    {
        BotLearningModel* model = it->second.get();
        model->epsilon = ::std::max(model->epsilon * _epsilonDecay, _epsilonMin);
    }
}

BehaviorAdaptation::BotLearningModel* BehaviorAdaptation::GetOrCreateModel(uint32_t botGuid)
{
    auto it = _botModels.find(botGuid);
    if (it == _botModels.end())
    {
        auto model = ::std::make_unique<BotLearningModel>();
        InitializeModel(model.get());
        model->epsilon = _epsilon;
        model->episodeCount = 0;
        model->stepCount = 0;
        model->metrics.Reset();

        auto* modelPtr = model.get();
        _botModels[botGuid] = ::std::move(model);
        return modelPtr;
    }
    return it->second.get();
}

void BehaviorAdaptation::InitializeModel(BotLearningModel* model)
{
    if (!model)
        return;

    // Initialize based on selected algorithm
    switch (_algorithm)
    {
        case LearningAlgorithm::Q_LEARNING:
        case LearningAlgorithm::DEEP_Q_NETWORK:
            model->qFunction = ::std::make_unique<QFunction>(STATE_SIZE, ACTION_SIZE);
            break;
        case LearningAlgorithm::POLICY_GRADIENT:
        case LearningAlgorithm::ACTOR_CRITIC:
            model->policyNetwork = ::std::make_unique<PolicyNetwork>(STATE_SIZE, ACTION_SIZE);
            break;
        default:
            // Default to Q-learning
            model->qFunction = ::std::make_unique<QFunction>(STATE_SIZE, ACTION_SIZE);
            break;
    }
}

void BehaviorAdaptation::NormalizeFeatures(::std::vector<float>& features) const
{
    // Simple min-max normalization to [0, 1]
    for (float& f : features)
    {
        f = ::std::clamp(f, 0.0f, 1.0f);
    }
}

::std::vector<Experience> BehaviorAdaptation::SampleBatch(const ::std::deque<Experience>& buffer, size_t batchSize)
{
    ::std::vector<Experience> batch;
    batch.reserve(batchSize);

    if (buffer.size() <= batchSize)
    {
        batch.assign(buffer.begin(), buffer.end());
    }
    else
    {
        // Uniform random sampling
        ::std::random_device rd;
        ::std::mt19937 gen(rd());
        ::std::uniform_int_distribution<size_t> dist(0, buffer.size() - 1);

        ::std::unordered_set<size_t> selectedIndices;
        while (batch.size() < batchSize)
        {
            size_t idx = dist(gen);
            if (selectedIndices.insert(idx).second)
            {
                batch.push_back(buffer[idx]);
            }
        }
    }

    return batch;
}

void BehaviorAdaptation::UpdateCollectiveKnowledge()
{
    if (!_collectiveModel || _sharedExperiences.empty())
        return;

    MEASURE_PERFORMANCE(MetricType::AI_DECISION_TIME, 0, "UpdateCollectiveKnowledge");

    // Sample batch from shared experiences
    ::std::vector<Experience> batch = SampleBatch(_sharedExperiences, 64);

    // Train collective model
    for (const Experience& exp : batch)
    {
        // Create target for supervised learning from successful experiences
        ::std::vector<float> target(ACTION_SIZE, 0.0f);
        if (exp.action < ACTION_SIZE)
        {
            target[exp.action] = ::std::max(0.0f, exp.reward);  // Use reward as confidence
        }

        _collectiveModel->Train(exp.state, target, _learningRate * 0.1f);  // Lower learning rate for stability
    }
}

BehaviorAdaptation::LearningMetrics BehaviorAdaptation::GetMetrics(uint32_t botGuid) const
{
    ::std::lock_guard lock(_modelsMutex);

    auto it = _botModels.find(botGuid);
    if (it != _botModels.end())
    {
        return it->second->metrics;
    }

    return LearningMetrics();
}

void BehaviorAdaptation::ResetMetrics(uint32_t botGuid)
{
    ::std::lock_guard lock(_modelsMutex);

    auto it = _botModels.find(botGuid);
    if (it != _botModels.end())
    {
        it->second->metrics.Reset();
    }
}

// ScopedLearningSession Implementation
ScopedLearningSession::ScopedLearningSession(uint32_t botGuid, BotAI* ai)
    : _botGuid(botGuid), _ai(ai), _cumulativeReward(0.0f), _committed(false)
{
    _initialState = sBehaviorAdaptation.ExtractStateFeatures(ai, ai ? ai->GetBot() : nullptr);
}

ScopedLearningSession::~ScopedLearningSession()
{
    if (!_committed)
        Commit();
}

void ScopedLearningSession::RecordAction(const ::std::string& action, bool success)
{
    _action = action;
    _cumulativeReward += success ? 1.0f : -0.5f;
}

void ScopedLearningSession::RecordReward(float reward)
{
    _cumulativeReward += reward;
}

void ScopedLearningSession::Commit()
{
    if (_committed || _action.empty())
        return;

    Experience exp;
    exp.state = _initialState;
    exp.action = sBehaviorAdaptation.MapActionToIndex(_action);
    exp.reward = _cumulativeReward;
    exp.nextState = sBehaviorAdaptation.ExtractStateFeatures(_ai, _ai ? _ai->GetBot() : nullptr);
    exp.terminal = false;
    exp.timestamp = ::std::chrono::steady_clock::now().time_since_epoch().count();

    sBehaviorAdaptation.RecordExperience(_botGuid, exp);
    _committed = true;
}

} // namespace Playerbot