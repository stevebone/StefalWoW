/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "PlayerPatternRecognition.h"
#include "Player.h"
#include "Unit.h"
#include "Group.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "Log.h"
#include "World.h"
#include "Performance/BotPerformanceMonitor.h"
#include "AI/BotAI.h"
#include "BehaviorAdaptation.h"
#include <cmath>
#include <algorithm>
#include <numeric>

namespace Playerbot
{

// Helper functions
float CosineSimilarity(const ::std::vector<float>& a, const ::std::vector<float>& b)
{
    if (a.size() != b.size() || a.empty())
        return 0.0f;

    float dotProduct = 0.0f;
    float normA = 0.0f;
    float normB = 0.0f;

    for (size_t i = 0; i < a.size(); ++i)
    {
        dotProduct += a[i] * b[i];
        normA += a[i] * a[i];
        normB += b[i] * b[i];
    }

    if (normA == 0.0f || normB == 0.0f)
        return 0.0f;

    return dotProduct / (::std::sqrt(normA) * ::std::sqrt(normB));
}

float EuclideanDistance(const ::std::vector<float>& a, const ::std::vector<float>& b)
{
    if (a.size() != b.size())
        return ::std::numeric_limits<float>::max();

    float sum = 0.0f;
    for (size_t i = 0; i < a.size(); ++i)
    {
        float diff = a[i] - b[i];
        sum += diff * diff;
    }
    return ::std::sqrt(sum);
}

// PatternSignature Implementation
float PatternSignature::CalculateSimilarity(const ::std::vector<float>& other) const
{
    return CosineSimilarity(features, other);
}

// PlayerProfile Implementation
PlayerProfile::PlayerProfile(ObjectGuid guid)
    : _playerGuid(guid)
    , _archetype(PlayerArchetype::UNKNOWN)
    , _archetypeConfidence(0.0f)
    , _averageAPM(0.0f)
    , _movementVariance(0.0f)
    , _targetSwitchRate(0.0f)
    , _defensiveReactivity(0.0f)
    , _aggressionLevel(0.5f)
    , _survivalPriority(0.5f)
    , _averageSpeed(0.0f)
    , _positionEntropy(0.0f)
    , _combatEngagements(0)
    , _combatVictories(0)
    , _averageCombatDuration(0.0f)
    , _damageEfficiency(0.0f)
{
}

PlayerProfile::~PlayerProfile() = default;

void PlayerProfile::AddSample(const BehaviorSample& sample)
{
    _samples.push_back(sample);

    // Keep buffer size manageable
    if (_samples.size() > MAX_SAMPLES)
        _samples.pop_front();

    // Update spell usage tracking
    if (sample.spellId != 0)
    {
        _spellUsageCounts[sample.spellId]++;

        // Track spell sequences
    if (!_samples.empty() && _samples.size() >= 2)
        {
            auto prevSample = _samples[_samples.size() - 2];
            if (prevSample.spellId != 0)
            {
                _spellSequences.push_back({prevSample.spellId, sample.spellId});
            }
        }
    }

    // Update movement vectors
    if (_samples.size() >= 2)
    {
        auto& prev = _samples[_samples.size() - 2];
        float dx = sample.x - prev.x;
        float dy = sample.y - prev.y;
        float dz = sample.z - prev.z;
        _movementVectors.push_back({dx, dy, dz});

        // Calculate speed
        float timeDelta = (sample.timestamp - prev.timestamp) / 1000000.0f; // Convert to seconds
    if (timeDelta > 0)
        {
            float distance = ::std::sqrt(dx*dx + dy*dy + dz*dz);
            float speed = distance / timeDelta;
            _averageSpeed = _averageSpeed * 0.95f + speed * 0.05f; // Exponential moving average
        }
    }

    // Update combat metrics
    if (sample.isInCombat && !_samples.empty())
    {
        auto& prev = _samples[_samples.size() - 2];
        if (!prev.isInCombat)
        {
            _combatEngagements++;
        }
    }

    // Periodically update patterns and metrics
    if (_samples.size() % 10 == 0)
    {
        ExtractMovementPatterns();
        ExtractAbilityPatterns();
        ExtractTargetingPatterns();
        CalculateBehaviorMetrics();
        UpdateArchetype();
    }
}

void PlayerProfile::UpdateArchetype()
{
    _archetype = ClassifyArchetype();

    // Calculate confidence based on sample size and consistency
    float sampleConfidence = ::std::min(1.0f, _samples.size() / 100.0f);
    float consistencyFactor = 1.0f; // Could be calculated based on variance in metrics
    _archetypeConfidence = sampleConfidence * consistencyFactor;
}

void PlayerProfile::ExtractMovementPatterns()
{
    if (_movementVectors.size() < 10)
        return;

    // Calculate movement variance
    float sumX = 0, sumY = 0, sumZ = 0;
    float sumX2 = 0, sumY2 = 0, sumZ2 = 0;

    for (const auto& vec : _movementVectors)
    {
        sumX += vec[0];
        sumY += vec[1];
        sumZ += vec[2];
        sumX2 += vec[0] * vec[0];
        sumY2 += vec[1] * vec[1];
        sumZ2 += vec[2] * vec[2];
    }

    size_t n = _movementVectors.size();
    float varX = (sumX2 / n) - (sumX / n) * (sumX / n);
    float varY = (sumY2 / n) - (sumY / n) * (sumY / n);
    float varZ = (sumZ2 / n) - (sumZ / n) * (sumZ / n);

    _movementVariance = (varX + varY + varZ) / 3.0f;

    // Calculate position entropy (measure of randomness)
    // Higher entropy = more unpredictable movement
    _positionEntropy = ::std::log(1.0f + _movementVariance);
}

void PlayerProfile::ExtractAbilityPatterns()
{
    if (_spellUsageCounts.empty())
        return;

    // Find most common spells
    ::std::vector<::std::pair<uint32_t, uint32_t>> sortedSpells(_spellUsageCounts.begin(), _spellUsageCounts.end());
    ::std::sort(sortedSpells.begin(), sortedSpells.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });

    // Create ability pattern signature
    PatternSignature abilitySig;
    abilitySig.type = PatternType::ABILITY_USAGE;
    abilitySig.confidence = _archetypeConfidence;
    abilitySig.occurrences = _samples.size();
    abilitySig.lastSeen = ::std::chrono::steady_clock::now();

    // Build feature vector from top spells
    for (size_t i = 0; i < ::std::min<size_t>(10, sortedSpells.size()); ++i)
    {
        abilitySig.features.push_back(static_cast<float>(sortedSpells[i].first));
        abilitySig.features.push_back(static_cast<float>(sortedSpells[i].second) / _samples.size());
    }

    _patterns[PatternType::ABILITY_USAGE].push_back(abilitySig);

    // Detect combo sequences
    if (_spellSequences.size() >= 5)
    {
        ::std::unordered_map<::std::pair<uint32_t, uint32_t>, uint32_t,
            [](const ::std::pair<uint32_t, uint32_t>& p) {
                return p.first ^ (p.second << 16);
            }> sequenceCounts;

        for (const auto& seq : _spellSequences)
            sequenceCounts[seq]++;

        PatternSignature comboSig;
        comboSig.type = PatternType::COMBO_SEQUENCE;
        comboSig.confidence = 0.8f;
        comboSig.occurrences = _spellSequences.size();

        for (const auto& [seq, count] : sequenceCounts)
        {
            if (count >= 3) // Repeated at least 3 times
            {
                comboSig.features.push_back(static_cast<float>(seq.first));
                comboSig.features.push_back(static_cast<float>(seq.second));
                comboSig.features.push_back(static_cast<float>(count) / _spellSequences.size());
            }
        }

        if (!comboSig.features.empty())
            _patterns[PatternType::COMBO_SEQUENCE].push_back(comboSig);
    }
}

void PlayerProfile::ExtractTargetingPatterns()
{
    if (_samples.size() < 10)
        return;

    // Count target switches
    uint32_t targetSwitches = 0;
    ObjectGuid lastTarget;

    for (const auto& sample : _samples)
    {
        if (sample.targetGuid != lastTarget && !sample.targetGuid.IsEmpty())
        {
            targetSwitches++;
            lastTarget = sample.targetGuid;
        }
    }

    float timePeriod = (_samples.back().timestamp - _samples.front().timestamp) / 60000000.0f; // Minutes
    if (timePeriod > 0)
        _targetSwitchRate = targetSwitches / timePeriod;

    // Create targeting pattern
    PatternSignature targetSig;
    targetSig.type = PatternType::TARGET_SELECTION;
    targetSig.confidence = 0.7f;
    targetSig.features.push_back(_targetSwitchRate);
    targetSig.features.push_back(static_cast<float>(targetSwitches));

    _patterns[PatternType::TARGET_SELECTION].push_back(targetSig);
}

void PlayerProfile::CalculateBehaviorMetrics()
{
    if (_samples.empty())
        return;

    // Calculate APM (Actions Per Minute)
    uint32_t actionCount = 0;
    for (const auto& sample : _samples)
    {
        if (sample.spellId != 0 || sample.isMoving)
            actionCount++;
    }

    float timePeriod = (_samples.back().timestamp - _samples.front().timestamp) / 60000000.0f; // Minutes
    if (timePeriod > 0)
        _averageAPM = actionCount / timePeriod;

    // Calculate defensive reactivity
    float lowHealthSamples = 0;
    float defensiveActions = 0;

    for (const auto& sample : _samples)
    {
        if (sample.healthPct < 50.0f)
        {
            lowHealthSamples++;
            if (sample.healingDone > 0 || sample.damageTaken < sample.damageDealt * 0.5f)
                defensiveActions++;
        }
    }

    if (lowHealthSamples > 0)
        _defensiveReactivity = defensiveActions / lowHealthSamples;

    // Calculate aggression level
    float totalDamage = 0;
    float totalHealing = 0;

    for (const auto& sample : _samples)
    {
        totalDamage += sample.damageDealt;
        totalHealing += sample.healingDone;
    }

    if (totalDamage + totalHealing > 0)
        _aggressionLevel = totalDamage / (totalDamage + totalHealing);

    // Calculate survival priority
    float avgHealthPct = 0;
    for (const auto& sample : _samples)
        avgHealthPct += sample.healthPct;

    avgHealthPct /= _samples.size();
    _survivalPriority = avgHealthPct / 100.0f;

    // Calculate damage efficiency
    float totalResourceUsed = 0;
    for (const auto& sample : _samples)
        totalResourceUsed += (100.0f - sample.resourcePct);

    if (totalResourceUsed > 0)
        _damageEfficiency = totalDamage / totalResourceUsed;
}

PlayerArchetype PlayerProfile::ClassifyArchetype() const
{
    // Simple classification based on behavioral metrics
    if (_aggressionLevel > 0.7f && _targetSwitchRate < 2.0f)
        return PlayerArchetype::AGGRESSIVE;

    if (_defensiveReactivity > 0.7f && _survivalPriority > 0.7f)
        return PlayerArchetype::DEFENSIVE;

    if (_aggressionLevel < 0.3f && _samples.size() > 0)
    {
        float healingRatio = 0;
        for (const auto& sample : _samples)
        {
            if (sample.healingDone > 0)
                healingRatio++;
        }
        healingRatio /= _samples.size();
        if (healingRatio > 0.5f)
            return PlayerArchetype::SUPPORTIVE;
    }

    if (_targetSwitchRate > 5.0f)
        return PlayerArchetype::OPPORTUNISTIC;

    if (_movementVariance > 100.0f && _positionEntropy > 2.0f)
        return PlayerArchetype::TACTICAL;

    if (_movementVariance < 10.0f && _averageAPM > 30.0f)
        return PlayerArchetype::CONSISTENT;

    // Check for adaptive behavior (high variance in patterns)
    if (_patterns.size() > 3)
        return PlayerArchetype::ADAPTIVE;

    return PlayerArchetype::UNKNOWN;
}

uint32_t PlayerProfile::PredictNextSpell() const
{
    if (_spellSequences.empty())
        return 0;

    // Get the last spell cast
    uint32_t lastSpell = 0;
    for (auto it = _samples.rbegin(); it != _samples.rend(); ++it)
    {
        if (it->spellId != 0)
        {
            lastSpell = it->spellId;
            break;
        }
    }

    if (lastSpell == 0)
        return 0;

    // Find most likely next spell based on sequences
    ::std::unordered_map<uint32_t, uint32_t> nextSpellCounts;
    for (const auto& seq : _spellSequences)
    {
        if (seq.first == lastSpell)
            nextSpellCounts[seq.second]++;
    }

    if (nextSpellCounts.empty())
        return 0;

    // Return most frequent next spell
    auto maxIt = ::std::max_element(nextSpellCounts.begin(), nextSpellCounts.end(),
        [](const auto& a, const auto& b) { return a.second < b.second; });

    return maxIt->first;
}

Position PlayerProfile::PredictNextPosition(float deltaTime) const
{
    Position predictedPos;

    if (_samples.empty())
        return predictedPos;

    const auto& lastSample = _samples.back();
    predictedPos.m_positionX = lastSample.x;
    predictedPos.m_positionY = lastSample.y;
    predictedPos.m_positionZ = lastSample.z;
    predictedPos.SetOrientation(lastSample.orientation);

    // Simple linear prediction based on average velocity
    if (_movementVectors.size() >= 5)
    {
        float avgDx = 0, avgDy = 0, avgDz = 0;
        size_t count = ::std::min<size_t>(5, _movementVectors.size());

        for (size_t i = _movementVectors.size() - count; i < _movementVectors.size(); ++i)
        {
            avgDx += _movementVectors[i][0];
            avgDy += _movementVectors[i][1];
            avgDz += _movementVectors[i][2];
        }

        avgDx /= count;
        avgDy /= count;
        avgDz /= count;

        predictedPos.m_positionX += avgDx * deltaTime;
        predictedPos.m_positionY += avgDy * deltaTime;
        predictedPos.m_positionZ += avgDz * deltaTime;
    }

    return predictedPos;
}

float PlayerProfile::CalculateSimilarity(const PlayerProfile& other) const
{
    float similarity = 0.0f;
    float weightSum = 0.0f;

    // Compare archetypes
    if (_archetype == other._archetype && _archetype != PlayerArchetype::UNKNOWN)
    {
        similarity += 0.3f;
    }
    weightSum += 0.3f;

    // Compare behavioral metrics
    float metricsSim = 0.0f;
    metricsSim += 1.0f - ::std::abs(_averageAPM - other._averageAPM) / 100.0f;
    metricsSim += 1.0f - ::std::abs(_movementVariance - other._movementVariance) / 200.0f;
    metricsSim += 1.0f - ::std::abs(_targetSwitchRate - other._targetSwitchRate) / 10.0f;
    metricsSim += 1.0f - ::std::abs(_defensiveReactivity - other._defensiveReactivity);
    metricsSim += 1.0f - ::std::abs(_aggressionLevel - other._aggressionLevel);
    metricsSim /= 5.0f;

    similarity += metricsSim * 0.4f;
    weightSum += 0.4f;

    // Compare spell usage patterns
    float spellSim = 0.0f;
    uint32_t commonSpells = 0;
    for (const auto& [spellId, count] : _spellUsageCounts)
    {
        auto it = other._spellUsageCounts.find(spellId);
        if (it != other._spellUsageCounts.end())
        {
            commonSpells++;
        }
    }

    if (!_spellUsageCounts.empty() && !other._spellUsageCounts.empty())
    {
        spellSim = static_cast<float>(commonSpells) /
                  ::std::max(_spellUsageCounts.size(), other._spellUsageCounts.size());
    }

    similarity += spellSim * 0.3f;
    weightSum += 0.3f;

    return weightSum > 0 ? similarity / weightSum : 0.0f;
}

// BehaviorCluster Implementation
BehaviorCluster::BehaviorCluster() = default;
BehaviorCluster::~BehaviorCluster() = default;

void BehaviorCluster::AddProfile(::std::shared_ptr<PlayerProfile> profile)
{
    if (!profile)
        return;

    _profiles[profile->_playerGuid] = profile;

    // Re-cluster if we have enough profiles
    if (_profiles.size() >= _k && _profiles.size() % 10 == 0)
    {
        UpdateClusters();
    }
}

void BehaviorCluster::UpdateClusters()
{
    if (_profiles.size() < _k)
        return;

    InitializeCentroids();

    for (uint32_t iter = 0; iter < _maxIterations; ++iter)
    {
        AssignToClusters();
        UpdateCentroids();

        if (HasConverged())
            break;
    }
}

void BehaviorCluster::InitializeCentroids()
{
    _clusters.clear();
    _clusters.resize(_k);

    // K-means++ initialization
    ::std::vector<::std::shared_ptr<PlayerProfile>> profileVec;
    for (const auto& [guid, profile] : _profiles)
        profileVec.push_back(profile);

    ::std::random_device rd;
    ::std::mt19937 gen(rd());
    ::std::uniform_int_distribution<size_t> dis(0, profileVec.size() - 1);

    // Choose first centroid randomly
    _clusters[0].centroid = profileVec[dis(gen)];

    // Choose remaining centroids with probability proportional to distance
    for (size_t i = 1; i < _k; ++i)
    {
        ::std::vector<float> distances;
        float totalDistance = 0;

        for (const auto& profile : profileVec)
        {
            float minDist = ::std::numeric_limits<float>::max();
            for (uint32_t j = 0; j < i; ++j)
            {
                float dist = 1.0f - profile->CalculateSimilarity(*_clusters[j].centroid);
                minDist = ::std::min(minDist, dist);
            }
            distances.push_back(minDist * minDist);
            totalDistance += minDist * minDist;
        }

        ::std::discrete_distribution<> weightedDis(distances.begin(), distances.end());
        _clusters[i].centroid = profileVec[weightedDis(gen)];
    }
}

void BehaviorCluster::AssignToClusters()
{
    // Clear current assignments
    for (auto& cluster : _clusters)
        cluster.members.clear();
    _profileToCluster.clear();

    // Assign each profile to nearest centroid
    for (const auto& [guid, profile] : _profiles)
    {
        float maxSimilarity = -1.0f;
        size_t bestCluster = 0;

        for (size_t i = 0; i < _clusters.size(); ++i)
        {
            if (!_clusters[i].centroid)
                continue;

            float sim = profile->CalculateSimilarity(*_clusters[i].centroid);
            if (sim > maxSimilarity)
            {
                maxSimilarity = sim;
                bestCluster = i;
            }
        }

        _clusters[bestCluster].members.push_back(profile);
        _profileToCluster[guid] = bestCluster;
    }
}

// PlayerPatternRecognition Implementation
PlayerPatternRecognition::PlayerPatternRecognition() : _initialized(false)
{
}

PlayerPatternRecognition::~PlayerPatternRecognition()
{
    Shutdown();
}

bool PlayerPatternRecognition::Initialize()
{
    if (_initialized)
        return true;

    TC_LOG_INFO("playerbot.pattern", "Initializing Player Pattern Recognition System");

    _behaviorCluster = ::std::make_unique<BehaviorCluster>();

    _initialized = true;
    TC_LOG_INFO("playerbot.pattern", "Player Pattern Recognition System initialized successfully");
    return true;
}

void PlayerPatternRecognition::Shutdown()
{
    if (!_initialized)
        return;

    TC_LOG_INFO("playerbot.pattern", "Shutting down Player Pattern Recognition System");

    ::std::lock_guard profileLock(_profilesMutex);
    ::std::lock_guard clusterLock(_clusterMutex);

    _profiles.clear();
    _behaviorCluster.reset();
    _globalPatterns.clear();
    _archetypePatterns.clear();
    _metaPatterns.clear();
    _predictionHistory.clear();

    _initialized = false;
}

void PlayerPatternRecognition::CreateProfile(Player* player)
{
    if (!player)
        return;

    ::std::lock_guard lock(_profilesMutex);

    ObjectGuid guid = player->GetGUID();
    if (_profiles.find(guid) == _profiles.end())
    {
        auto profile = ::std::make_shared<PlayerProfile>(guid);
        _profiles[guid] = profile;
        _metrics.profilesTracked++;

        // Add to clustering system
    if (_behaviorCluster)
        {
            ::std::lock_guard clusterLock(_clusterMutex);
            _behaviorCluster->AddProfile(profile);
        }
    }
}

::std::shared_ptr<PlayerProfile> PlayerPatternRecognition::GetProfile(ObjectGuid guid) const
{
    ::std::lock_guard lock(_profilesMutex);

    auto it = _profiles.find(guid);
    if (it != _profiles.end())
        return it->second;

    return nullptr;
}

void PlayerPatternRecognition::RecordPlayerBehavior(Player* player)
{
    if (!player || !_initialized)
        return;

    MEASURE_PERFORMANCE(MetricType::AI_DECISION_TIME, player->GetGUID().GetCounter(), "RecordBehavior");

    auto profile = GetProfile(player->GetGUID());
    if (!profile)
    {
        CreateProfile(player);
        profile = GetProfile(player->GetGUID());
    }

    if (profile)
    {
        BehaviorSample sample = CreateBehaviorSample(player);
        profile->AddSample(sample);
        _metrics.samplesProcessed++;
    }
}

BehaviorSample PlayerPatternRecognition::CreateBehaviorSample(Player* player) const
{
    BehaviorSample sample;

    sample.timestamp = ::std::chrono::steady_clock::now().time_since_epoch().count();

    Position pos = player->GetPosition();
    sample.x = pos.GetPositionX();
    sample.y = pos.GetPositionY();
    sample.z = pos.GetPositionZ();
    sample.orientation = pos.GetOrientation();

    sample.healthPct = player->GetHealthPct();
    sample.resourcePct = player->GetPowerPct(player->GetPowerType());
    sample.isMoving = player->isMoving();
    sample.isInCombat = player->IsInCombat();
    if (player->GetVictim())
        sample.targetGuid = player->GetVictim()->GetGUID();
    // Count auras
    sample.auraCount = player->GetAppliedAuras().size();

    // Note: Damage/healing tracking would need integration with combat system
    sample.damageDealt = 0;
    sample.damageTaken = 0;
    sample.healingDone = 0;

    return sample;
}

void PlayerPatternRecognition::RecordCombatAction(Player* player, uint32_t spellId, Unit* target)
{
    if (!player || !_initialized)
        return;
    auto profile = GetProfile(player->GetGUID());
    if (!profile)
        return;

    BehaviorSample sample = CreateBehaviorSample(player);
    sample.spellId = spellId;
    if (target)
        sample.targetGuid = target->GetGUID();

    profile->AddSample(sample);
}

void PlayerPatternRecognition::ApplyPlayerStyle(Player* bot, ObjectGuid templatePlayerGuid)
{
    if (!bot || !_initialized)
        return;

    auto templateProfile = GetProfile(templatePlayerGuid);
    if (!templateProfile)
        return;

    // Apply movement patterns
    float movementVariance = templateProfile->GetMovementVariance();

    // Apply ability patterns
    uint32_t predictedSpell = templateProfile->PredictNextSpell();
    if (predictedSpell != 0)
    {
        // Bot AI would use this prediction
        TC_LOG_DEBUG("playerbot.pattern", "Bot %s mimicking spell %u from player template",
            bot->GetName().c_str(), predictedSpell);
    }

    // Apply targeting patterns
    float targetSwitchRate = templateProfile->GetTargetSwitchRate();

    // Apply archetype behavior
    ApplyArchetypeStyle(bot, templateProfile->GetArchetype());
}

void PlayerPatternRecognition::ApplyArchetypeStyle(Player* bot, PlayerArchetype archetype)
{
    if (!bot)
        return;

    // Adjust bot behavior based on archetype
    switch (archetype)
    {
        case PlayerArchetype::AGGRESSIVE:
            // Set aggressive behavior parameters
            TC_LOG_DEBUG("playerbot.pattern", "Bot %s adopting aggressive style", bot->GetName().c_str());
            break;
        case PlayerArchetype::DEFENSIVE:
            // Set defensive behavior parameters
            TC_LOG_DEBUG("playerbot.pattern", "Bot %s adopting defensive style", bot->GetName().c_str());
            break;
        case PlayerArchetype::SUPPORTIVE:
            // Set supportive behavior parameters
            TC_LOG_DEBUG("playerbot.pattern", "Bot %s adopting supportive style", bot->GetName().c_str());
            break;
        case PlayerArchetype::TACTICAL:
            // Set tactical behavior parameters
            TC_LOG_DEBUG("playerbot.pattern", "Bot %s adopting tactical style", bot->GetName().c_str());
            break;
        default:
            break;
    }
}

PlayerPatternRecognition::PredictionResult PlayerPatternRecognition::PredictPlayerAction(
    Player* player, float timeHorizon)
{
    PredictionResult result;
    result.confidence = 0.0f;

    if (!player || !_initialized)
        return result;

    auto profile = GetProfile(player->GetGUID());
    if (!profile)
        return result;

    // Predict next spell
    result.predictedAction = profile->PredictNextSpell();

    // Predict next position
    result.predictedPosition = profile->PredictNextPosition(timeHorizon);

    // Calculate confidence based on profile data
    result.confidence = profile->GetArchetypeConfidence();

    // Store prediction for validation
    PredictionValidation validation;
    validation.playerGuid = player->GetGUID();
    validation.prediction = result;
    validation.timestamp = ::std::chrono::steady_clock::now();
    validation.validated = false;

    _predictionHistory.push_back(validation);
    if (_predictionHistory.size() > MAX_PREDICTION_HISTORY)
        _predictionHistory.pop_front();

    return result;
}

bool PlayerPatternRecognition::IsAnomalousBehavior(Player* player) const
{
    if (!player || !_initialized)
        return false;

    return GetAnomalyScore(player) > ANOMALY_THRESHOLD;
}

float PlayerPatternRecognition::GetAnomalyScore(Player* player) const
{
    if (!player || !_initialized)
        return 0.0f;

    auto profile = GetProfile(player->GetGUID());
    if (!profile)
        return 0.0f;

    // Calculate anomaly score based on deviation from expected patterns
    float score = 0.0f;

    // Check for unusual APM
    float apm = profile->GetAverageAPM();
    if (apm > 200 || apm < 5)  // Suspiciously high or low APM
        score += 2.0f;

    // Check for teleportation-like movement
    float movementVar = profile->GetMovementVariance();
    if (movementVar > 1000)  // Very high variance suggests teleporting
        score += 3.0f;

    // Check for impossible reaction times
    float defensiveReact = profile->GetDefensiveReactivity();
    if (defensiveReact > 0.95f)  // Near-perfect defensive reactions
        score += 2.0f;

    return score;
}

void PlayerPatternRecognition::UpdateMetaPatterns()
{
    if (!_initialized)
        return;

    auto now = ::std::chrono::steady_clock::now();
    auto timeSinceLastUpdate = ::std::chrono::duration_cast<::std::chrono::hours>(
        now - _lastMetaUpdate).count();

    if (timeSinceLastUpdate < 1)  // Update hourly
        return;

    TC_LOG_INFO("playerbot.pattern", "Updating meta patterns");

    ::std::lock_guard lock(_profilesMutex);

    // Analyze all profiles to identify meta patterns
    ::std::unordered_map<PlayerArchetype, uint32_t> archetypeCounts;
    ::std::unordered_map<uint32_t, uint32_t> popularSpells;

    for (const auto& [guid, profile] : _profiles)
    {
        archetypeCounts[profile->GetArchetype()]++;

        // Aggregate spell usage
    for (const auto& pattern : profile->GetPatterns(PatternType::ABILITY_USAGE))
        {
            for (size_t i = 0; i < pattern.features.size(); i += 2)
            {
                if (i + 1 < pattern.features.size())
                {
                    uint32_t spellId = static_cast<uint32_t>(pattern.features[i]);
                    popularSpells[spellId]++;
                }
            }
        }
    }

    // Create meta patterns from aggregated data
    _metaPatterns.clear();

    // Most popular archetype pattern
    auto maxArchetype = ::std::max_element(archetypeCounts.begin(), archetypeCounts.end(),
        [](const auto& a, const auto& b) { return a.second < b.second; });

    if (maxArchetype != archetypeCounts.end())
    {
        PatternSignature metaPattern;
        metaPattern.type = PatternType::ABILITY_USAGE;
        metaPattern.confidence = 0.8f;
        metaPattern.features.push_back(static_cast<float>(maxArchetype->first));
        metaPattern.lastSeen = now;
        _metaPatterns.push_back(metaPattern);
    }

    _lastMetaUpdate = now;
}

void PlayerPatternRecognition::AdaptBotsToMeta()
{
    if (!_initialized || _metaPatterns.empty())
        return;

    TC_LOG_INFO("playerbot.pattern", "Adapting bots to current meta patterns");

    // This would integrate with BehaviorAdaptation to adjust bot strategies
    for (const auto& metaPattern : _metaPatterns)
    {
        // Apply meta strategies to bot learning
        // This would be implemented in conjunction with BehaviorAdaptation
    }
}

// ScopedPatternRecording Implementation
ScopedPatternRecording::ScopedPatternRecording(Player* player)
    : _player(player)
{
    _startTime = ::std::chrono::steady_clock::now();
}

ScopedPatternRecording::~ScopedPatternRecording()
{
    if (_player && !_samples.empty())
    {
        auto profile = sPlayerPatternRecognition.GetProfile(_player->GetGUID());
        if (profile)
        {
            for (const auto& sample : _samples)
                profile->AddSample(sample);
        }
    }
}

void ScopedPatternRecording::RecordAction(uint32_t actionId)
{
    if (!_player)
        return;

    BehaviorSample sample = sPlayerPatternRecognition.CreateBehaviorSample(_player);
    sample.spellId = actionId;
    _samples.push_back(sample);
}

void ScopedPatternRecording::RecordPosition()
{
    if (!_player)
        return;

    BehaviorSample sample = sPlayerPatternRecognition.CreateBehaviorSample(_player);
    _samples.push_back(sample);
}

} // namespace Playerbot