/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 */

#include "DecisionFusionSystem.h"
#include "GameTime.h"
#include "SpellHistory.h"
#include "../BehaviorPriorityManager.h"
#include "BotAI.h"
#include "BehaviorPriorityManager.h"
#include "ActionPriorityQueue.h"
#include "BehaviorTree.h"
#include "Common/ActionScoringEngine.h"
#include "Combat/AdaptiveBehaviorManager.h"
#include "Player.h"
#include "Group.h"
#include "ObjectAccessor.h"
#include "Unit.h"
#include "Log.h"
#include "Config/PlayerbotConfig.h"
#include <algorithm>
#include <unordered_map>
#include <sstream>

namespace Playerbot {
namespace bot { namespace ai {

// Constructor with default weights
DecisionFusionSystem::DecisionFusionSystem()
    : _urgencyThreshold(0.85f)
    , _debugLogging(false)
    , _stats{}
{
    // Load from configuration
    _debugLogging = sPlayerbotConfig->GetBool("Playerbot.AI.DecisionFusion.LogDecisions", false);
    _urgencyThreshold = sPlayerbotConfig->GetFloat("Playerbot.AI.DecisionFusion.UrgencyThreshold", 0.85f);

    // Load system weights from configuration (with defaults)
    _systemWeights[static_cast<size_t>(DecisionSource::BEHAVIOR_PRIORITY)] =
        sPlayerbotConfig->GetFloat("Playerbot.AI.DecisionFusion.Weight.BehaviorPriority", 0.25f);
    _systemWeights[static_cast<size_t>(DecisionSource::ACTION_PRIORITY)] =
        sPlayerbotConfig->GetFloat("Playerbot.AI.DecisionFusion.Weight.ActionPriority", 0.15f);
    _systemWeights[static_cast<size_t>(DecisionSource::BEHAVIOR_TREE)] =
        sPlayerbotConfig->GetFloat("Playerbot.AI.DecisionFusion.Weight.BehaviorTree", 0.30f);
    _systemWeights[static_cast<size_t>(DecisionSource::ADAPTIVE_BEHAVIOR)] =
        sPlayerbotConfig->GetFloat("Playerbot.AI.DecisionFusion.Weight.AdaptiveBehavior", 0.10f);
    _systemWeights[static_cast<size_t>(DecisionSource::WEIGHTING_SYSTEM)] =
        sPlayerbotConfig->GetFloat("Playerbot.AI.DecisionFusion.Weight.WeightingSystem", 0.20f);

    // Normalize weights to ensure they sum to 1.0
    NormalizeWeights();
}

void DecisionFusionSystem::SetSystemWeights(
    float behaviorPriorityWeight,
    float actionPriorityWeight,
    float behaviorTreeWeight,
    float adaptiveWeight,
    float weightingSystemWeight)
{
    _systemWeights[static_cast<size_t>(DecisionSource::BEHAVIOR_PRIORITY)] = behaviorPriorityWeight;
    _systemWeights[static_cast<size_t>(DecisionSource::ACTION_PRIORITY)] = actionPriorityWeight;
    _systemWeights[static_cast<size_t>(DecisionSource::BEHAVIOR_TREE)] = behaviorTreeWeight;
    _systemWeights[static_cast<size_t>(DecisionSource::ADAPTIVE_BEHAVIOR)] = adaptiveWeight;
    _systemWeights[static_cast<size_t>(DecisionSource::WEIGHTING_SYSTEM)] = weightingSystemWeight;

    NormalizeWeights();
}

void DecisionFusionSystem::NormalizeWeights()
{
    float sum = 0.0f;
    for (float weight : _systemWeights)
        sum += weight;

    if (sum > 0.0f)
    {
        for (float& weight : _systemWeights)

            weight /= sum;
    }
}

::std::vector<DecisionVote> DecisionFusionSystem::CollectVotes(::Playerbot::BotAI* ai, CombatContext context)
{
    ::std::vector<DecisionVote> votes;
    votes.reserve(static_cast<size_t>(DecisionSource::MAX));

    if (!ai)
        return votes;

    Player* bot = ai->GetBot();
    if (!bot)
        return votes;

    if (_debugLogging)
    {
        TC_LOG_DEBUG("playerbot", "DecisionFusionSystem: Collecting votes for bot {}, context {}",

            bot->GetName(), static_cast<uint32>(context));
    }

    // ========================================================================
    // 1. BEHAVIOR PRIORITY MANAGER - Strategy-level decisions
    // ========================================================================
    if (auto priorityMgr = ai->GetPriorityManager())
    {
        BehaviorPriority activePriority = priorityMgr->GetActivePriority();

        // Convert BehaviorPriority to actionId (use priority enum value as action ID)
        // Confidence based on how long this priority has been active
        // Urgency based on priority level (COMBAT=1.0, FOLLOW=0.5, SOLO=0.1)
        float confidence = 0.8f; // BehaviorPriority is reliable
        float urgency = 0.0f;

        switch (activePriority)
        {

            case ::Playerbot::BehaviorPriority::COMBAT:
                urgency = 1.0f; break;
            case ::Playerbot::BehaviorPriority::FLEEING:
                urgency = 0.95f; break;
            case ::Playerbot::BehaviorPriority::CASTING:
                urgency = 0.7f; break;
            case ::Playerbot::BehaviorPriority::FOLLOW:
                urgency = 0.5f; break;
            case ::Playerbot::BehaviorPriority::MOVEMENT:
                urgency = 0.4f; break;
            case ::Playerbot::BehaviorPriority::GATHERING:
                urgency = 0.3f; break;
            case ::Playerbot::BehaviorPriority::TRADING:
                urgency = 0.2f; break;
            case ::Playerbot::BehaviorPriority::SOCIAL:
                urgency = 0.1f; break;
            case ::Playerbot::BehaviorPriority::SOLO:
                urgency = 0.1f; break;
            default:
                urgency = 0.0f; break;
        }

        if (urgency > 0.0f)
        {

            DecisionVote vote(

                DecisionSource::BEHAVIOR_PRIORITY,

                static_cast<uint32>(activePriority),

                nullptr, // BehaviorPriority doesn't specify targets

                confidence,

                urgency,

                "BehaviorPriority: Active priority"

            );

            votes.push_back(vote);


            if (_debugLogging)

                LogVote(vote, vote.CalculateWeightedScore(_systemWeights[static_cast<size_t>(DecisionSource::BEHAVIOR_PRIORITY)]));
        }
    }

    // ========================================================================
    // 2. ACTION PRIORITY QUEUE - Spell priority management
    // ========================================================================
    if (auto priorityQueue = ai->GetActionPriorityQueue())
    {
        DecisionVote vote = priorityQueue->GetVote(bot, ai->GetTargetUnit(), context);
        if (vote.actionId != 0)
        {

            votes.push_back(vote);


            if (_debugLogging)

                LogVote(vote, vote.CalculateWeightedScore(_systemWeights[static_cast<size_t>(DecisionSource::ACTION_PRIORITY)]));
        }
    }

    // ========================================================================
    // 3. BEHAVIOR TREE - Hierarchical combat decisions
    // ========================================================================
    if (auto behaviorTree = ai->GetBehaviorTree())
    {
        // Tick the behavior tree to execute current frame
        behaviorTree->Tick(bot, ai->GetTargetUnit());

        // Get vote from tree if it has a recommendation
        DecisionVote vote = behaviorTree->GetVote(bot, ai->GetTargetUnit(), context);
        if (vote.actionId != 0)
        {

            votes.push_back(vote);


            if (_debugLogging)

                LogVote(vote, vote.CalculateWeightedScore(_systemWeights[static_cast<size_t>(DecisionSource::BEHAVIOR_TREE)]));
        }
    }

    // ========================================================================
    // 4. ADAPTIVE BEHAVIOR MANAGER - Role-specific recommendations
    // ========================================================================
    // Note: AdaptiveBehaviorManager::GetRecommendedAction() is implemented (Phase 5)
    // but AdaptiveBehaviorManager is currently nested in CombatBehaviorIntegration
    // which is not directly accessible from BotAI. Future work will either:
    // a) Add AdaptiveBehaviorManager to BotAI directly, or
    // b) Add CombatBehaviorIntegration accessor to BotAI
    //
    // When integrated, it will provide:
    // - Role-based action recommendations (tank/healer/DPS)
    // - Emergency condition detection (emergency heal/tank)
    // - Strategy-based confidence/urgency adjustments
    // - Context-aware reasoning (raid/dungeon/PvP)
    //
    // For now, this vote source is disabled pending architectural integration.

    // ========================================================================
    // 5. ACTION SCORING ENGINE - Utility-based scoring
    // ========================================================================
    // Phase 5 Integration: Use ActionPriorityQueue's registered spells as candidates
    // ActionScoringEngine scores them using multi-criteria utility evaluation
    if (auto priorityQueue = ai->GetActionPriorityQueue())
    {
        // Get all available spells from ActionPriorityQueue
        ::std::vector<uint32> candidateSpells = priorityQueue->GetPrioritizedSpells(bot, ai->GetTargetUnit(), context);

        if (!candidateSpells.empty() && candidateSpells.size() <= 50) // Limit to top 50 for performance
        {
            // Determine bot role for scoring
            BotRole role = DetermineBotRole(bot);

            // Create scoring engine for current context and role
            ActionScoringEngine scorer(role, context);

            // Score all candidate spells using utility-based evaluation
            auto scores = scorer.ScoreActions(candidateSpells,
                [this, bot, ai, &context](ScoringCategory category, uint32 spellId) -> float
                {
                    // Category evaluator: returns 0.0-1.0 value for each category
                    return EvaluateScoringCategory(category, bot, ai->GetTargetUnit(), spellId, context);
                });

            // Get best action from scored list
            uint32 bestAction = scorer.GetBestAction(scores);

            if (bestAction != 0)
            {
                // Find the score details for the best action
                auto it = ::std::find_if(scores.begin(), scores.end(),
                    [bestAction](const ActionScore& s) { return s.actionId == bestAction; });

                if (it != scores.end())
                {
                    // Create vote with utility-based confidence
                    // Normalize total score to 0-1 confidence (typical scores: 0-500)
                    float confidence = ::std::min(it->totalScore / 500.0f, 1.0f);

                    // Urgency based on survival and group protection scores
                    float survivalScore = it->GetCategoryScore(ScoringCategory::SURVIVAL);
                    float protectionScore = it->GetCategoryScore(ScoringCategory::GROUP_PROTECTION);
                    float urgency = ::std::min((survivalScore + protectionScore) / 2.0f, 1.0f);

                    DecisionVote vote(
                        DecisionSource::WEIGHTING_SYSTEM,
                        bestAction,
                        ai->GetTargetUnit(),
                        confidence,
                        urgency,
                        "ActionScoring: Utility-based selection (score: " + ::std::to_string(static_cast<int>(it->totalScore)) + ")"
                    );

                    votes.push_back(vote);

                    if (_debugLogging)
                    {
                        TC_LOG_DEBUG("playerbot", "ActionScoring: Selected spell {} with score {:.1f}",
                            bestAction, it->totalScore);
                        LogVote(vote, vote.CalculateWeightedScore(_systemWeights[static_cast<size_t>(DecisionSource::WEIGHTING_SYSTEM)]));
                    }
                }
            }
        }
    }

    if (_debugLogging)
    {
        TC_LOG_DEBUG("playerbot", "DecisionFusionSystem: Collected {} vote(s)", votes.size());
    }

    return votes;
}

DecisionResult DecisionFusionSystem::FuseDecisions(const ::std::vector<DecisionVote>& votes)
{
    DecisionResult result;

    if (votes.empty())
        return result;

    // Increment decision counter
    ++_stats.totalDecisions;

    // Check for unanimous decisions
    if (AreVotesUnanimous(votes))
        ++_stats.unanimousDecisions;
    else
        ++_stats.conflictResolutions;

    // Step 1: Check for urgent overrides
    const DecisionVote* urgentVote = FindHighestUrgencyVote(votes);
    if (urgentVote && urgentVote->urgency >= _urgencyThreshold)
    {
        // Urgent action takes priority regardless of consensus
        result.actionId = urgentVote->actionId;
        result.target = urgentVote->target;
        result.consensusScore = urgentVote->CalculateWeightedScore(_systemWeights[static_cast<size_t>(urgentVote->source)]);
        result.contributingVotes.push_back(*urgentVote);
        result.fusionReasoning = "URGENT: " + urgentVote->reasoning;

        ++_stats.urgencyOverrides;
        ++_stats.systemWins[static_cast<size_t>(urgentVote->source)];

        if (_debugLogging)

            LogDecision(result, votes);

        return result;
    }

    // Step 2: Group votes by action ID and calculate consensus scores
    ::std::unordered_map<uint32, ::std::vector<const DecisionVote*>> votesByAction;
    for (const auto& vote : votes)
    {
        if (vote.actionId != 0)

            votesByAction[vote.actionId].push_back(&vote);
    }

    // Step 3: Calculate consensus score for each action
    struct ActionConsensus
    {
        uint32 actionId;
        Unit* target;
        float consensusScore;
        ::std::vector<DecisionVote> contributingVotes;
        DecisionSource primarySource;
    };

    ::std::vector<ActionConsensus> consensuses;
    consensuses.reserve(votesByAction.size());

    for (const auto& [actionId, actionVotes] : votesByAction)
    {
        ActionConsensus consensus;
        consensus.actionId = actionId;
        consensus.target = nullptr;
        consensus.consensusScore = 0.0f;

        float highestVoteScore = 0.0f;

        for (const DecisionVote* vote : actionVotes)
        {

            float weightedScore = vote->CalculateWeightedScore(_systemWeights[static_cast<size_t>(vote->source)]);

            consensus.consensusScore += weightedScore;

            consensus.contributingVotes.push_back(*vote);

            // Track highest scoring vote for target selection
    if (weightedScore > highestVoteScore)

            {

                highestVoteScore = weightedScore;

                consensus.target = vote->target;

                consensus.primarySource = vote->source;

            }


            if (_debugLogging)

                LogVote(*vote, weightedScore);
        }

        consensuses.push_back(::std::move(consensus));
    }

    // Step 4: Select action with highest consensus score
    if (consensuses.empty())
        return result;

    auto bestConsensus = ::std::max_element(consensuses.begin(), consensuses.end(),
        [](const ActionConsensus& a, const ActionConsensus& b) {

            return a.consensusScore < b.consensusScore;
        });

    // Step 5: Build final result
    result.actionId = bestConsensus->actionId;
    result.target = bestConsensus->target;
    result.consensusScore = bestConsensus->consensusScore;
    result.contributingVotes = ::std::move(bestConsensus->contributingVotes);

    // Build reasoning string
    ::std::ostringstream reasoning;
    reasoning << "Consensus from " << result.contributingVotes.size() << " system(s): ";
    for (size_t i = 0; i < result.contributingVotes.size(); ++i)
    {
        if (i > 0)

            reasoning << ", ";
        reasoning << GetSourceName(result.contributingVotes[i].source);
    }
    result.fusionReasoning = reasoning.str();

    // Update statistics
    ++_stats.systemWins[static_cast<size_t>(bestConsensus->primarySource)];

    if (_debugLogging)
        LogDecision(result, votes);

    return result;
}

void DecisionFusionSystem::LogVote(const DecisionVote& vote, float weightedScore) const
{
    TC_LOG_DEBUG("playerbot", "DecisionFusion Vote: [{}] Action {} | Confidence {:.2f} | Urgency {:.2f} | Weighted {:.2f} | Reason: {}",
        GetSourceName(vote.source),
        vote.actionId,
        vote.confidence,
        vote.urgency,
        weightedScore,
        vote.reasoning);
}

void DecisionFusionSystem::LogDecision(const DecisionResult& result, const ::std::vector<DecisionVote>& allVotes) const
{
    if (!result.IsValid())
    {
        TC_LOG_DEBUG("playerbot", "DecisionFusion Result: NO VALID DECISION (received {} votes)", allVotes.size());
        return;
    }

    TC_LOG_DEBUG("playerbot", "DecisionFusion Result: Action {} | Score {:.2f} | Votes {} | Reason: {}",
        result.actionId,
        result.consensusScore,
        result.contributingVotes.size(),
        result.fusionReasoning);

    // Log contributing systems
    ::std::ostringstream systems;
    for (size_t i = 0; i < result.contributingVotes.size(); ++i)
    {
        if (i > 0)

            systems << ", ";
        systems << GetSourceName(result.contributingVotes[i].source)

                << " (" << result.contributingVotes[i].confidence << ")";
    }

    TC_LOG_DEBUG("playerbot", "DecisionFusion Contributing Systems: {}", systems.str());
}

bool DecisionFusionSystem::AreVotesUnanimous(const ::std::vector<DecisionVote>& votes) const
{
    if (votes.size() <= 1)
        return true;

    uint32 firstAction = 0;
    for (const auto& vote : votes)
    {
        if (vote.actionId == 0)

            continue;

        if (firstAction == 0)

            firstAction = vote.actionId;
        else if (firstAction != vote.actionId)

            return false;
    }

    return true;
}

const DecisionVote* DecisionFusionSystem::FindHighestUrgencyVote(const ::std::vector<DecisionVote>& votes) const
{
    const DecisionVote* highestUrgency = nullptr;
    float maxUrgency = 0.0f;

    for (const auto& vote : votes)
    {
        if (vote.urgency > maxUrgency)
        {

            maxUrgency = vote.urgency;

            highestUrgency = &vote;
        }
    }

    return highestUrgency;
}

const char* DecisionFusionSystem::GetSourceName(DecisionSource source)
{
    switch (source)
    {
        case DecisionSource::BEHAVIOR_PRIORITY:  return "BehaviorPriority";
        case DecisionSource::ACTION_PRIORITY:    return "ActionPriority";
        case DecisionSource::BEHAVIOR_TREE:      return "BehaviorTree";
        case DecisionSource::ADAPTIVE_BEHAVIOR:  return "AdaptiveBehavior";
        case DecisionSource::WEIGHTING_SYSTEM:   return "WeightingSystem";

        default:
        return "Unknown";
    }
}

// ============================================================================
// ACTION SCORING ENGINE HELPER FUNCTIONS
// ============================================================================

BotRole DecisionFusionSystem::DetermineBotRole(Player* bot) const
{
    if (!bot)
        return BotRole::RANGED_DPS; // Default

    // Get player's class and spec
    Classes playerClass = static_cast<Classes>(bot->GetClass());
    uint32 spec = static_cast<uint32>(bot->GetPrimarySpecialization());

    // Determine role based on class and spec
    switch (playerClass)
    {
        case CLASS_WARRIOR:

            return (spec == 2) ? BotRole::TANK : BotRole::MELEE_DPS; // Prot = Tank

        case CLASS_PALADIN:

            if (spec == 1) return BotRole::HEALER;  // Holy
    if (spec == 2) return BotRole::TANK;
            // Prot

            return BotRole::MELEE_DPS;
            // Ret

        case CLASS_HUNTER:

            return BotRole::RANGED_DPS;

        case CLASS_ROGUE:

            return BotRole::MELEE_DPS;

        case CLASS_PRIEST:

            return (spec == 3) ? BotRole::RANGED_DPS : BotRole::HEALER; // Shadow = DPS

        case CLASS_DEATH_KNIGHT:

            return (spec == 1) ? BotRole::TANK : BotRole::MELEE_DPS; // Blood = Tank

        case CLASS_SHAMAN:

            if (spec == 3) return BotRole::HEALER;
            // Resto
    if (spec == 1) return BotRole::RANGED_DPS;  // Ele

            return BotRole::MELEE_DPS;
            // Enh

        case CLASS_MAGE:
        case CLASS_WARLOCK:

            return BotRole::RANGED_DPS;

        case CLASS_DRUID:

            if (spec == 0) return BotRole::RANGED_DPS;  // Balance
    if (spec == 1) return BotRole::MELEE_DPS;   // Feral (DPS)
    if (spec == 2) return BotRole::TANK;
            // Feral (Tank) / Guardian

            return BotRole::HEALER;
            // Resto

        default:

            return BotRole::RANGED_DPS;
    }
}

float DecisionFusionSystem::EvaluateScoringCategory(
    ScoringCategory category,
    Player* bot,
    Unit* target,
    uint32 spellId,
    CombatContext context) const
{
    if (!bot)
        return 0.0f;

    // Evaluate each category and return 0.0-1.0 value
    switch (category)
    {
        case ScoringCategory::SURVIVAL:
        {
            // Higher score when bot's health is lower

            float healthPct = bot->GetHealthPct();

            if (healthPct < 20.0f)

                return 1.0f;  // Critical
    if (healthPct < 40.0f)

                return 0.8f;  // Urgent
    if (healthPct < 60.0f)

                return 0.5f;  // Moderate
    if (healthPct < 80.0f)

                return 0.2f;  // Low

            return 0.0f;
            // No survival concern
        }

        case ScoringCategory::GROUP_PROTECTION:
        {
            // Score based on group members' health and threat
    if (Group* group = bot->GetGroup())

            {

                uint32 membersNeedingHelp = 0;

                uint32 totalMembers = 0;


                for (Group::MemberSlot const& slot : group->GetMemberSlots())

                {

                    if (Player* member = ObjectAccessor::FindPlayer(slot.guid))

                    {

                        if (member->IsAlive())

                        {

                            totalMembers++;

                            if (member->GetHealthPct() < 60.0f)

                                membersNeedingHelp++;

                        }

                    }

                }


                if (totalMembers > 0)

                {

                    float helpRatio = static_cast<float>(membersNeedingHelp) / static_cast<float>(totalMembers);

                    return ::std::min(helpRatio, 1.0f);

                }

            }

            return 0.0f;
        }

        case ScoringCategory::DAMAGE_OPTIMIZATION:
        {
            // Score based on target health and DPS opportunity
    if (!target || !target->IsAlive())

                return 0.0f;


            float targetHealthPct = target->GetHealthPct();

            // Execute range (< 20% HP) = high priority
    if (targetHealthPct < 20.0f)

                return 0.9f;

            // Normal DPS window
    if (targetHealthPct > 80.0f)

                return 0.7f; // Fresh target, good DPS opportunity

            // Mid-fight

            return 0.5f;
        }

        case ScoringCategory::RESOURCE_EFFICIENCY:
        {
            // Score based on mana/resource levels

            Powers powerType = bot->GetPowerType();

            if (powerType == POWER_MANA)

            {

                float manaPct = bot->GetPowerPct(POWER_MANA);

                if (manaPct < 20.0f)

                    return 1.0f; // Very high priority to conserve
    if (manaPct < 40.0f)

                    return 0.7f; // High priority
    if (manaPct < 60.0f)

                    return 0.4f; // Moderate

                return 0.1f;
                // Plenty of mana

            }

            else

            {
                // For non-mana users (warriors, rogues, etc.), always low priority

                return 0.1f;

            }
        }

        case ScoringCategory::POSITIONING_MECHANICS:
        {
            // Score based on positioning needs
            // For now, basic implementation based on range
    if (!target)

                return 0.0f;


            float distance = bot->GetDistance(target);

            // Melee range
    if (distance < 5.0f)

                return 0.2f; // Good positioning for melee

            // Mid range
    if (distance < 30.0f)

                return 0.5f; // Good positioning for ranged

            // Too far

            return 0.8f; // Need to reposition
        }

        case ScoringCategory::STRATEGIC_VALUE:
        {
            // Score based on context and fight phase
    switch (context)

            {

                case CombatContext::RAID_HEROIC:
                case CombatContext::RAID_NORMAL:

                    return 0.8f; // High strategic importance in raids


                case CombatContext::DUNGEON_BOSS:

                    return 0.6f; // Moderate strategic importance


                case CombatContext::PVP_ARENA:

                case CombatContext::PVP_BG:

                    return 0.7f; // High strategic importance in PvP


                default:

                    return 0.3f; // Lower strategic importance in solo/trash

            }
        }

        default:

            return 0.0f;
    }
}

}} // namespace bot::ai
} // namespace Playerbot
