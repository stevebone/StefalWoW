/*
 * Copyright (C) 2024+ TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ActionScoringEngine.h"
#include "Config/PlayerbotConfig.h"
#include <cmath>
#include <iomanip>

namespace Playerbot {
namespace bot { namespace ai {

// Role Multiplier Table [role][category]
// Order: Survival, GroupProtection, Damage, Resource, Positioning, Strategic
const std::array<std::array<float, 6>, 4> ActionScoringEngine::ROLE_MULTIPLIERS = {{
    // TANK
    { 1.5f, 1.2f, 0.8f, 0.9f, 1.2f, 1.0f },
    // HEALER
    { 1.3f, 2.0f, 0.3f, 1.5f, 1.1f, 1.0f },
    // MELEE_DPS
    { 1.0f, 0.8f, 1.5f, 1.0f, 1.1f, 1.0f },
    // RANGED_DPS
    { 1.0f, 0.8f, 1.5f, 1.0f, 1.1f, 1.0f }
}};

// Context Modifier Table [context][category]
// Order: Survival, GroupProtection, Damage, Resource, Positioning, Strategic
const std::array<std::array<float, 6>, 8> ActionScoringEngine::CONTEXT_MULTIPLIERS = {{
    // SOLO
    { 1.3f, 0.5f, 1.2f, 0.9f, 1.0f, 0.8f },
    // GROUP
    { 1.1f, 1.3f, 1.2f, 1.0f, 1.1f, 1.0f },
    // DUNGEON_TRASH
    { 1.0f, 1.2f, 1.3f, 1.0f, 1.1f, 0.9f },
    // DUNGEON_BOSS
    { 1.1f, 1.5f, 1.2f, 1.1f, 1.4f, 1.3f },
    // RAID_NORMAL
    { 1.0f, 1.8f, 1.0f, 1.2f, 1.5f, 1.5f },
    // RAID_HEROIC
    { 1.2f, 2.0f, 1.1f, 1.4f, 1.8f, 1.8f },
    // PVP_ARENA
    { 1.4f, 1.6f, 1.3f, 0.8f, 1.3f, 1.2f },
    // PVP_BG
    { 1.1f, 1.3f, 1.2f, 0.9f, 1.2f, 1.4f }
}};

ActionScoringEngine::ActionScoringEngine(BotRole role, CombatContext context)
    : _role(role)
    , _context(context)
    , _weights()
    , _debugLogging(false)
    , _effectiveWeights{}
{
    // Load weights from configuration
    ResetToDefaultWeights();
    RecalculateEffectiveWeights();
}

ActionScore ActionScoringEngine::ScoreAction(
    uint32 actionId,
    const std::function<float(ScoringCategory)>& categoryEvaluator) const
{
    ActionScore result;
    result.actionId = actionId;
    result.totalScore = 0.0f;

    // Evaluate each category
    for (size_t i = 0; i < static_cast<size_t>(ScoringCategory::MAX); ++i)
    {
        ScoringCategory category = static_cast<ScoringCategory>(i);

        // Get category value from evaluator (0.0-1.0)
        float categoryValue = categoryEvaluator(category);
        categoryValue = std::clamp(categoryValue, 0.0f, 1.0f);

        // Calculate category score: effectiveWeight × categoryValue
        float categoryScore = _effectiveWeights[i] * categoryValue;

        // Apply diminishing returns
        categoryScore = ApplyDiminishingReturns(categoryScore, category);

        result.SetCategoryScore(category, categoryScore);
        result.totalScore += categoryScore;
    }

    // Generate debug info if logging enabled
    if (_debugLogging)
    {
        result.debugInfo = GetScoreBreakdown(result);
        TC_LOG_DEBUG("playerbot.weighting", "ActionScoringEngine: Scored action {} = {:.2f}\n{}",
            actionId, result.totalScore, result.debugInfo);
    }

    return result;
}

std::vector<ActionScore> ActionScoringEngine::ScoreActions(
    const std::vector<uint32>& actionIds,
    const std::function<float(ScoringCategory, uint32)>& categoryEvaluator) const
{
    std::vector<ActionScore> results;
    results.reserve(actionIds.size());

    for (uint32 actionId : actionIds)
    {
        ActionScore score = ScoreAction(actionId, [&categoryEvaluator, actionId](ScoringCategory cat) {
            return categoryEvaluator(cat, actionId);
        });
        results.push_back(score);
    }

    return results;
}

uint32 ActionScoringEngine::GetBestAction(const std::vector<ActionScore>& scores) const
{
    if (scores.empty())
        return 0;

    auto maxIt = std::max_element(scores.begin(), scores.end(),
        [](const ActionScore& a, const ActionScore& b) {
            return a.totalScore < b.totalScore;
        });

    // Return 0 if best score is negligible
    if (maxIt->totalScore <= 0.01f)
        return 0;

    return maxIt->actionId;
}

std::vector<uint32> ActionScoringEngine::GetTopActions(const std::vector<ActionScore>& scores, size_t count) const
{
    if (scores.empty() || count == 0)
        return {};

    // Create a copy and sort by score (highest first)
    std::vector<ActionScore> sortedScores = scores;
    std::sort(sortedScores.begin(), sortedScores.end(),
        [](const ActionScore& a, const ActionScore& b) {
            return a.totalScore > b.totalScore;
        });

    // Extract top N action IDs
    std::vector<uint32> topActions;
    topActions.reserve(std::min(count, sortedScores.size()));

    for (size_t i = 0; i < std::min(count, sortedScores.size()); ++i)
    {
        if (sortedScores[i].totalScore > 0.01f) // Skip negligible scores
            topActions.push_back(sortedScores[i].actionId);
    }

    return topActions;
}

std::string ActionScoringEngine::GetScoreBreakdown(const ActionScore& score) const
{
    std::ostringstream oss;

    oss << "Action " << score.actionId << " (Total: " << std::fixed << std::setprecision(2) << score.totalScore << ")\n";
    oss << "  Role: " << GetRoleName(_role) << ", Context: " << GetContextName(_context) << "\n";
    oss << "  Category Breakdown:\n";

    for (size_t i = 0; i < static_cast<size_t>(ScoringCategory::MAX); ++i)
    {
        ScoringCategory category = static_cast<ScoringCategory>(i);
        float categoryScore = score.GetCategoryScore(category);

        if (categoryScore > 0.01f) // Only show contributing categories
        {
            oss << "    " << std::left << std::setw(20) << GetCategoryName(category)
                << ": " << std::right << std::setw(6) << std::fixed << std::setprecision(2)
                << categoryScore
                << " (weight: " << std::setw(6) << _effectiveWeights[i] << ")\n";
        }
    }

    return oss.str();
}

void ActionScoringEngine::SetRole(BotRole role)
{
    if (_role != role)
    {
        _role = role;
        RecalculateEffectiveWeights();

        if (_debugLogging)
        {
            TC_LOG_DEBUG("playerbot.weighting", "ActionScoringEngine: Role changed to {}", GetRoleName(role));
        }
    }
}

void ActionScoringEngine::SetContext(CombatContext context)
{
    if (_context != context)
    {
        _context = context;
        RecalculateEffectiveWeights();

        if (_debugLogging)
        {
            TC_LOG_DEBUG("playerbot.weighting", "ActionScoringEngine: Context changed to {}", GetContextName(context));
        }
    }
}

void ActionScoringEngine::SetCustomWeights(const ScoringWeights& weights)
{
    _weights = weights;
    RecalculateEffectiveWeights();

    if (_debugLogging)
    {
        TC_LOG_DEBUG("playerbot.weighting", "ActionScoringEngine: Custom weights applied");
    }
}

void ActionScoringEngine::ResetToDefaultWeights()
{
    // Load from configuration
    _weights.survival = sPlayerbotConfig->GetFloat("Playerbot.AI.Weighting.SurvivalWeight", 200.0f);
    _weights.groupProtection = sPlayerbotConfig->GetFloat("Playerbot.AI.Weighting.GroupProtectionWeight", 180.0f);
    _weights.damageOptimization = sPlayerbotConfig->GetFloat("Playerbot.AI.Weighting.DamageWeight", 150.0f);
    _weights.resourceEfficiency = sPlayerbotConfig->GetFloat("Playerbot.AI.Weighting.ResourceWeight", 100.0f);
    _weights.positioningMechanics = sPlayerbotConfig->GetFloat("Playerbot.AI.Weighting.PositioningWeight", 120.0f);
    _weights.strategicValue = sPlayerbotConfig->GetFloat("Playerbot.AI.Weighting.StrategicWeight", 80.0f);

    RecalculateEffectiveWeights();
}

float ActionScoringEngine::GetEffectiveWeight(ScoringCategory category) const
{
    return _effectiveWeights[static_cast<size_t>(category)];
}

float ActionScoringEngine::GetRoleMultiplier(BotRole role, ScoringCategory category)
{
    size_t roleIdx = static_cast<size_t>(role);
    size_t catIdx = static_cast<size_t>(category);

    if (roleIdx >= ROLE_MULTIPLIERS.size() || catIdx >= 6)
        return 1.0f;

    return ROLE_MULTIPLIERS[roleIdx][catIdx];
}

float ActionScoringEngine::GetContextMultiplier(CombatContext context, ScoringCategory category)
{
    size_t contextIdx = static_cast<size_t>(context);
    size_t catIdx = static_cast<size_t>(category);

    if (contextIdx >= CONTEXT_MULTIPLIERS.size() || catIdx >= 6)
        return 1.0f;

    return CONTEXT_MULTIPLIERS[contextIdx][catIdx];
}

const char* ActionScoringEngine::GetCategoryName(ScoringCategory category)
{
    switch (category)
    {
        case ScoringCategory::SURVIVAL:              return "Survival";
        case ScoringCategory::GROUP_PROTECTION:      return "Group Protection";
        case ScoringCategory::DAMAGE_OPTIMIZATION:   return "Damage";
        case ScoringCategory::RESOURCE_EFFICIENCY:   return "Resource";
        case ScoringCategory::POSITIONING_MECHANICS: return "Positioning";
        case ScoringCategory::STRATEGIC_VALUE:       return "Strategic";
        default:                                      return "Unknown";
    }
}

const char* ActionScoringEngine::GetContextName(CombatContext context)
{
    switch (context)
    {
        case CombatContext::SOLO:           return "Solo";
        case CombatContext::GROUP:          return "Group";
        case CombatContext::DUNGEON_TRASH:  return "Dungeon Trash";
        case CombatContext::DUNGEON_BOSS:   return "Dungeon Boss";
        case CombatContext::RAID_NORMAL:    return "Raid Normal";
        case CombatContext::RAID_HEROIC:    return "Raid Heroic";
        case CombatContext::PVP_ARENA:      return "PvP Arena";
        case CombatContext::PVP_BG:         return "PvP BG";
        default:                             return "Unknown";
    }
}

const char* ActionScoringEngine::GetRoleName(BotRole role)
{
    switch (role)
    {
        case BotRole::TANK:        return "Tank";
        case BotRole::HEALER:      return "Healer";
        case BotRole::MELEE_DPS:   return "Melee DPS";
        case BotRole::RANGED_DPS:  return "Ranged DPS";
        default:                    return "Unknown";
    }
}

void ActionScoringEngine::RecalculateEffectiveWeights()
{
    for (size_t i = 0; i < static_cast<size_t>(ScoringCategory::MAX); ++i)
    {
        ScoringCategory category = static_cast<ScoringCategory>(i);

        // Get base weight
        float baseWeight = _weights.GetWeight(category);

        // Apply role multiplier
        float roleMultiplier = GetRoleMultiplier(_role, category);

        // Apply context modifier
        float contextModifier = GetContextMultiplier(_context, category);

        // Calculate effective weight
        _effectiveWeights[i] = baseWeight * roleMultiplier * contextModifier;
    }

    if (_debugLogging)
    {
        TC_LOG_DEBUG("playerbot.weighting", "ActionScoringEngine: Effective weights recalculated");
        for (size_t i = 0; i < static_cast<size_t>(ScoringCategory::MAX); ++i)
        {
            ScoringCategory category = static_cast<ScoringCategory>(i);
            TC_LOG_DEBUG("playerbot.weighting", "  {}: {:.2f}", GetCategoryName(category), _effectiveWeights[i]);
        }
    }
}

float ActionScoringEngine::ApplyDiminishingReturns(float rawScore, ScoringCategory /*category*/) const
{
    // Simple logarithmic diminishing returns to prevent extreme scores
    // Formula: score × (1 + log(1 + score/100))
    // This keeps scores in reasonable ranges while allowing high scores
    if (rawScore <= 0.0f)
        return 0.0f;

    float normalizedScore = rawScore / 100.0f;
    float diminished = rawScore * (1.0f + std::log1p(normalizedScore));

    return diminished;
}

}} // namespace bot::ai
} // namespace Playerbot
