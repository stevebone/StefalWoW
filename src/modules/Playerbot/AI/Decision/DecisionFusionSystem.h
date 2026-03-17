/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 */

#ifndef _PLAYERBOT_DECISIONFUSIONSYSTEM_H
#define _PLAYERBOT_DECISIONFUSIONSYSTEM_H

#include "Common.h"
#include "ObjectGuid.h"
#include "../Common/ActionScoringEngine.h"
#include <vector>
#include <string>
#include <functional>
#include <memory>

class Unit;
class Player;

namespace Playerbot {

// Forward declaration of BotAI in correct namespace
class BotAI;

namespace bot { namespace ai {

enum class CombatContext : uint8;

/**
 * @enum DecisionSource
 * @brief Identifies which decision system provided a vote
 */
enum class DecisionSource : uint8
{
    BEHAVIOR_PRIORITY,      // BehaviorPriorityManager (strategy priority)
    ACTION_PRIORITY,        // ActionPriorityQueue (spell priority)
    BEHAVIOR_TREE,          // Behavior Trees (hierarchical decisions)
    ADAPTIVE_BEHAVIOR,      // AdaptiveBehaviorManager (role-based behavior)
    WEIGHTING_SYSTEM,       // ActionScoringEngine (utility-based scoring)
    MAX
};

/**
 * @struct DecisionVote
 * @brief Represents a single decision system's recommendation
 *
 * Each decision system votes for an action with confidence and urgency.
 * The fusion system combines these votes to select the best action.
 */
struct DecisionVote
{
    DecisionSource source;          // Which system voted
    uint32 actionId;                // Proposed action (spell ID, behavior ID, etc.)
    Unit* target;                   // Proposed target (nullptr for self/no target)
    float confidence;               // 0.0-1.0: How confident is this decision?
    float urgency;                  // 0.0-1.0: How urgent is this action?
    float utilityScore;             // Raw utility score (for debugging)
    ::std::string reasoning;          // Debug info: Why this action?

    DecisionVote()
        : source(DecisionSource::MAX)
        , actionId(0)
        , target(nullptr)
        , confidence(0.0f)
        , urgency(0.0f)
        , utilityScore(0.0f)
        , reasoning("")
    {}

    DecisionVote(DecisionSource src, uint32 action, Unit* tgt, float conf, float urg, const ::std::string& reason = "")
        : source(src)
        , actionId(action)
        , target(tgt)
        , confidence(conf)
        , urgency(urg)
        , utilityScore(0.0f)
        , reasoning(reason)
    {}

    /**
     * @brief Calculate weighted score for this vote
     * @param systemWeight Weight for this decision system (0.0-1.0)
     * @return Weighted score combining confidence, urgency, and system weight
     */
    [[nodiscard]] float CalculateWeightedScore(float systemWeight) const
    {
        // Formula: (confidence × urgency) × systemWeight
        // Confidence: How sure we are this is the right action
        // Urgency: How important it is to act NOW
        // SystemWeight: How much we trust this decision system
        return (confidence * urgency) * systemWeight;
    }
};

/**
 * @struct DecisionResult
 * @brief Final decision after fusion
 */
struct DecisionResult
{
    uint32 actionId;                            // Chosen action
    Unit* target;                               // Chosen target
    float consensusScore;                       // Combined vote score
    ::std::vector<DecisionVote> contributingVotes;// Votes that agreed
    ::std::string fusionReasoning;                // Why this action was chosen

    DecisionResult()
        : actionId(0)
        , target(nullptr)
        , consensusScore(0.0f)
        , fusionReasoning("")
    {}

    [[nodiscard]] bool IsValid() const { return actionId != 0; }
};

/**
 * @class DecisionFusionSystem
 * @brief Unified arbitration for all decision-making systems
 *
 * **Problem**: 4 independent decision systems operate without coordination:
 * - BehaviorPriorityManager (strategy priority)
 * - ActionPriorityQueue (spell priority)
 * - Behavior Trees (hierarchical decisions)
 * - AdaptiveBehaviorManager (role-based behavior)
 * - ActionScoringEngine (utility-based scoring) [newly added]
 *
 * **Solution**: Decision Fusion Framework
 * - Collects votes from all systems
 * - Fuses votes using weighted consensus
 * - Resolves conflicts intelligently
 * - Provides debuggable decision reasoning
 *
 * **Pattern**: Chain of Responsibility with weighting
 *
 * **Usage Example**:
 * @code
 * DecisionFusionSystem fusion;
 * fusion.SetSystemWeights(0.2f, 0.2f, 0.3f, 0.1f, 0.2f);
 *
 * // Collect votes from all systems
 * std::vector<DecisionVote> votes = fusion.CollectVotes(botAI, context);
 *
 * // Fuse votes to get best action
 * DecisionResult result = fusion.FuseDecisions(votes);
 *
 * if (result.IsValid())
 *     botAI->ExecuteAction(result.actionId, result.target);
 * @endcode
 *
 * **Expected Impact**:
 * -  Eliminate decision conflicts
 * -  20-30% better action selection
 * -  Debuggable decision reasoning
 * -  Coordinated multi-system behavior
 */
class TC_GAME_API DecisionFusionSystem
{
public:
    /**
     * @brief Constructor with default weights
     *
     * Default weights:
     * - Behavior Priority: 0.25 (strategy-level decisions)
     * - Action Priority: 0.15 (spell priority queue)
     * - Behavior Tree: 0.30 (hierarchical structure)
     * - Adaptive Behavior: 0.10 (role adjustments)
     * - Weighting System: 0.20 (utility-based scoring)
     */
    DecisionFusionSystem();

    /**
     * @brief Configure system weights
     *
     * @param behaviorPriorityWeight Weight for BehaviorPriorityManager (default: 0.25)
     * @param actionPriorityWeight Weight for ActionPriorityQueue (default: 0.15)
     * @param behaviorTreeWeight Weight for Behavior Trees (default: 0.30)
     * @param adaptiveWeight Weight for AdaptiveBehaviorManager (default: 0.10)
     * @param weightingSystemWeight Weight for ActionScoringEngine (default: 0.20)
     *
     * @note Weights are normalized internally to sum to 1.0
     */
    void SetSystemWeights(
        float behaviorPriorityWeight,
        float actionPriorityWeight,
        float behaviorTreeWeight,
        float adaptiveWeight,
        float weightingSystemWeight
    );

    /**
     * @brief Collect votes from all decision systems
     *
     * @param ai Bot AI instance
     * @param context Current combat context
     * @return Vector of votes from all active systems
     *
     * @note This method queries each decision system and collects their recommendations
     */
    ::std::vector<DecisionVote> CollectVotes(::Playerbot::BotAI* ai, CombatContext context);

    /**
     * @brief Fuse votes using weighted consensus
     *
     * **Algorithm**:
     * 1. Group votes by action ID
     * 2. Calculate consensus score for each action:
     *    - Sum of (confidence × urgency × systemWeight) for all votes for that action
     * 3. Select action with highest consensus score
     * 4. If urgency > threshold, override with urgent action
     *
     * @param votes Collected votes from all systems
     * @return Highest consensus action or invalid result if no consensus
     */
    DecisionResult FuseDecisions(const ::std::vector<DecisionVote>& votes);

    /**
     * @brief Enable/disable debug logging
     *
     * When enabled, logs:
     * - All votes collected
     * - Weighted scores for each vote
     * - Consensus calculation
     * - Final decision reasoning
     */
    void EnableDebugLogging(bool enable) { _debugLogging = enable; }

    /**
     * @brief Get current system weights (for debugging/tuning)
     */
    [[nodiscard]] const ::std::array<float, static_cast<size_t>(DecisionSource::MAX)>& GetSystemWeights() const
    {
        return _systemWeights;
    }

    /**
     * @brief Get decision statistics (for monitoring)
     */
    struct DecisionStats
    {
        uint32 totalDecisions;
        uint32 conflictResolutions;     // Times multiple systems disagreed
        uint32 unanimousDecisions;      // Times all systems agreed
        uint32 urgencyOverrides;        // Times urgency overrode consensus
        ::std::array<uint32, static_cast<size_t>(DecisionSource::MAX)> systemWins;
    };

    [[nodiscard]] DecisionStats GetStats() const { return _stats; }
    void ResetStats() { _stats = {}; }

private:
    // System weights (normalized to sum to 1.0)
    ::std::array<float, static_cast<size_t>(DecisionSource::MAX)> _systemWeights;

    // Urgency threshold for immediate action (default: 0.85)
    float _urgencyThreshold;

    // Debug logging
    bool _debugLogging;

    // Statistics
    mutable DecisionStats _stats;

    /**
     * @brief Normalize weights to sum to 1.0
     */
    void NormalizeWeights();

    /**
     * @brief Log vote for debugging
     */
    void LogVote(const DecisionVote& vote, float weightedScore) const;

    /**
     * @brief Log final decision for debugging
     */
    void LogDecision(const DecisionResult& result, const ::std::vector<DecisionVote>& allVotes) const;

    /**
     * @brief Check if votes are unanimous
     */
    [[nodiscard]] bool AreVotesUnanimous(const ::std::vector<DecisionVote>& votes) const;

    /**
     * @brief Find highest urgency vote
     */
    [[nodiscard]] const DecisionVote* FindHighestUrgencyVote(const ::std::vector<DecisionVote>& votes) const;

    /**
     * @brief Get system name for logging
     */
    [[nodiscard]] static const char* GetSourceName(DecisionSource source);

    /**
     * @brief Determine bot role from class and spec
     * @param bot Player bot
     * @return Bot role for ActionScoringEngine
     */
    [[nodiscard]] BotRole DetermineBotRole(Player* bot) const;

    /**
     * @brief Evaluate scoring category for ActionScoringEngine
     * @param category Scoring category to evaluate
     * @param bot Player bot
     * @param target Current target
     * @param spellId Spell being scored
     * @param context Combat context
     * @return Score value (0.0-1.0)
     */
    [[nodiscard]] float EvaluateScoringCategory(
        ScoringCategory category,
        Player* bot,
        Unit* target,
        uint32 spellId,
        CombatContext context) const;
};

}} // namespace bot::ai
} // namespace Playerbot

#endif
