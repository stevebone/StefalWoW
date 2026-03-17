/*
 * Copyright (C) 2024+ TrinityCore <http://www.trinitycore.org/>
 *
 * Phase 5 Enhancement: Action Priority Queue Implementation
 */

#include "ActionPriorityQueue.h"
#include "GameTime.h"
#include "SpellHistory.h"
#include "Map.h"
#include "DecisionFusionSystem.h"
#include "Player.h"
#include "Unit.h"
#include "SpellMgr.h"
#include "SpellInfo.h"
#include "Log.h"
#include <algorithm>

namespace Playerbot {
namespace bot { namespace ai {

// ============================================================================
// PrioritizedSpell Implementation
// ============================================================================

float PrioritizedSpell::CalculateEffectivePriority(Player* bot, Unit* target, CombatContext context) const
{
    if (!bot)
        return 0.0f;

    // Base priority (0.0-1.0 scale)
    float priority = static_cast<float>(basePriority) / 100.0f;

    // Apply priority multiplier
    priority *= priorityMultiplier;

    // Context-based adjustments
    switch (context)
    {
        case CombatContext::DUNGEON_BOSS:
        case CombatContext::RAID_HEROIC:
            // Boost offensive cooldowns in boss fights
    if (category == SpellCategory::OFFENSIVE)
                priority *= 1.2f;
            break;

        case CombatContext::DUNGEON_TRASH:
            // Boost AoE in trash packs
    if (category == SpellCategory::DAMAGE_AOE)
                priority *= 1.5f;
            break;

        case CombatContext::PVP_ARENA:
        case CombatContext::PVP_BG:
            // Boost defensive and CC in PvP
    if (category == SpellCategory::DEFENSIVE || category == SpellCategory::CROWD_CONTROL)
                priority *= 1.3f;
            break;

        default:
            break;
    }

    // Health-based priority adjustments
    if (bot->GetHealthPct() < 30.0f && category == SpellCategory::DEFENSIVE)
        priority *= 2.0f; // Double defensive priority at low health
    if (target && target->GetHealthPct() < 20.0f && category == SpellCategory::DAMAGE_SINGLE)
        priority *= 1.3f; // Boost execute-range damage

    // Clamp to 0.0-1.0 range
    return ::std::min(::std::max(priority, 0.0f), 1.0f);
}

bool PrioritizedSpell::AreConditionsMet(Player* bot, Unit* target) const
{
    for (const auto& condition : conditions)
    {
        if (!condition.condition(bot, target))
            return false;
    }
    return true;
}

// ============================================================================
// ActionPriorityQueue Implementation
// ============================================================================

ActionPriorityQueue::ActionPriorityQueue()
    : _debugLogging(false)
{
}

void ActionPriorityQueue::RegisterSpell(uint32 spellId, SpellPriority priority, SpellCategory category)
{
    // Check if spell is already registered
    if (FindSpell(spellId))
    {
        TC_LOG_WARN("playerbot", "ActionPriorityQueue: Spell {} already registered", spellId);
        return;
    }

    PrioritizedSpell spell(spellId, priority, category);
    _spells.push_back(spell);

    if (_debugLogging)
        TC_LOG_DEBUG("playerbot", "ActionPriorityQueue: Registered spell {} with priority {}",
            spellId, static_cast<uint32>(priority));
}

void ActionPriorityQueue::AddCondition(uint32 spellId, ::std::function<bool(Player*, Unit*)> condition, const ::std::string& description)
{
    PrioritizedSpell* spell = FindSpell(spellId);
    if (!spell)
    {
        TC_LOG_WARN("playerbot", "ActionPriorityQueue: Cannot add condition to unregistered spell {}", spellId);
        return;
    }

    spell->conditions.emplace_back(condition, description);

    if (_debugLogging)
        TC_LOG_DEBUG("playerbot", "ActionPriorityQueue: Added condition '{}' to spell {}", description, spellId);
}

void ActionPriorityQueue::SetPriorityMultiplier(uint32 spellId, float multiplier)
{
    PrioritizedSpell* spell = FindSpell(spellId);
    if (!spell)
    {
        TC_LOG_WARN("playerbot", "ActionPriorityQueue: Cannot set multiplier for unregistered spell {}", spellId);
        return;
    }

    spell->priorityMultiplier = multiplier;
}

uint32 ActionPriorityQueue::GetHighestPrioritySpell(Player* bot, Unit* target, CombatContext context) const
{
    if (!bot)
        return 0;

    float highestPriority = 0.0f;
    uint32 bestSpell = 0;

    for (const auto& spell : _spells)
    {
        // Check if spell is on cooldown
    if (IsOnCooldown(bot, spell.spellId))
            continue;

        // Check if bot has enough resources
    if (!HasEnoughResources(bot, spell.spellId))
            continue;

        // Check if target is valid
    if (!IsValidTarget(bot, target, spell.spellId))
            continue;

        // Check custom conditions
    if (!spell.AreConditionsMet(bot, target))
            continue;

        // Calculate effective priority
        float priority = spell.CalculateEffectivePriority(bot, target, context);

        if (priority > highestPriority)
        {
            highestPriority = priority;
            bestSpell = spell.spellId;
        }
    }

    if (_debugLogging && bestSpell != 0)
        TC_LOG_DEBUG("playerbot", "ActionPriorityQueue: Highest priority spell: {} (priority: {:.2f})",
            bestSpell, highestPriority);

    return bestSpell;
}

DecisionVote ActionPriorityQueue::GetVote(Player* bot, Unit* target, CombatContext context) const
{
    DecisionVote vote;
    vote.source = DecisionSource::ACTION_PRIORITY;

    uint32 bestSpell = GetHighestPrioritySpell(bot, target, context);
    if (bestSpell == 0)
        return vote; // No valid spell

    const PrioritizedSpell* spell = FindSpell(bestSpell);
    if (!spell)
        return vote;

    // Set vote parameters
    vote.actionId = bestSpell;
    vote.target = target;

    // Confidence based on priority level
    // EMERGENCY (100) -> 1.0 confidence
    // HIGH (70) -> 0.7 confidence
    // LOW (30) -> 0.3 confidence
    vote.confidence = static_cast<float>(spell->basePriority) / 100.0f;

    // Urgency based on spell category and situation
    if (spell->category == SpellCategory::DEFENSIVE && bot && bot->GetHealthPct() < 30.0f)
        vote.urgency = 0.95f; // Very urgent defensive
    else if (spell->basePriority == SpellPriority::EMERGENCY)
        vote.urgency = 1.0f;
    else if (spell->basePriority == SpellPriority::CRITICAL)
        vote.urgency = 0.85f;
    else if (spell->basePriority == SpellPriority::HIGH)
        vote.urgency = 0.7f;
    else if (spell->basePriority == SpellPriority::MEDIUM)
        vote.urgency = 0.5f;
    else
        vote.urgency = 0.3f;

    // Set reasoning
    vote.reasoning = "ActionPriorityQueue: Spell " + ::std::to_string(bestSpell);

    return vote;
}

::std::vector<uint32> ActionPriorityQueue::GetPrioritizedSpells(Player* bot, Unit* target, CombatContext context) const
{
    if (!bot)
        return {};

    struct SpellScore
    {
        uint32 spellId;
        float priority;
    };

    ::std::vector<SpellScore> scores;

    for (const auto& spell : _spells)
    {
        // Check availability
    if (IsOnCooldown(bot, spell.spellId))
            continue;
        if (!HasEnoughResources(bot, spell.spellId))
            continue;
        if (!IsValidTarget(bot, target, spell.spellId))
            continue;
        if (!spell.AreConditionsMet(bot, target))
            continue;

        float priority = spell.CalculateEffectivePriority(bot, target, context);
        scores.push_back({spell.spellId, priority});
    }

    // Sort by priority (highest first)
    ::std::sort(scores.begin(), scores.end(), [](const SpellScore& a, const SpellScore& b)
    {
        return a.priority > b.priority;
    });

    // Extract spell IDs
    ::std::vector<uint32> result;
    result.reserve(scores.size());
    for (const auto& score : scores)
        result.push_back(score.spellId);

    return result;
}

void ActionPriorityQueue::RecordCast(uint32 spellId)
{
    PrioritizedSpell* spell = FindSpell(spellId);
    if (spell)
        spell->lastCastTime = GameTime::GetGameTimeMS();
}

void ActionPriorityQueue::Clear()
{
    _spells.clear();
}

bool ActionPriorityQueue::IsOnCooldown(Player* bot, uint32 spellId) const
{
    if (!bot)
        return true;

    // Check spell cooldown using TrinityCore API
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, bot->GetMap()->GetDifficultyID());
    if (!spellInfo)
        return true;

    // Check if spell is on cooldown
    if (bot->GetSpellHistory()->HasCooldown(spellId))
        return true;

    // Check global cooldown
    if (bot->GetSpellHistory()->GetRemainingGlobalCooldown(spellInfo) > Milliseconds::zero())
        return true;

    return false;
}

bool ActionPriorityQueue::HasEnoughResources(Player* bot, uint32 spellId) const
{
    if (!bot)
        return false;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, bot->GetMap()->GetDifficultyID());
    if (!spellInfo)
        return false;

    // Check power costs
    auto costs = spellInfo->CalcPowerCost(bot, spellInfo->GetSchoolMask());
    for (auto const& cost : costs)
    {
        if (bot->GetPower(cost.Power) < cost.Amount)
            return false;
    }

    return true;
}

bool ActionPriorityQueue::IsValidTarget(Player* bot, Unit* target, uint32 spellId) const
{
    if (!bot)
        return false;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, bot->GetMap()->GetDifficultyID());
    if (!spellInfo)
        return false;

    // Self-cast spells
    if (spellInfo->HasAttribute(SPELL_ATTR0_IS_ABILITY))
    {
        if (!target)
            target = bot; // Default to self
    }

    if (!target)
        return false;

    // Check if target is valid
    if (!target->IsAlive() && !spellInfo->HasAttribute(SPELL_ATTR2_ALLOW_DEAD_TARGET))
        return false;

    // Check range
    float range = spellInfo->GetMaxRange(false, bot, nullptr);
    if (bot->GetDistance(target) > range)
        return false;

    // Check line of sight
    if (!bot->IsWithinLOSInMap(target))
        return false;

    return true;
}

PrioritizedSpell* ActionPriorityQueue::FindSpell(uint32 spellId)
{
    for (auto& spell : _spells)
    {
        if (spell.spellId == spellId)
            return &spell;
    }
    return nullptr;
}

const PrioritizedSpell* ActionPriorityQueue::FindSpell(uint32 spellId) const
{
    for (const auto& spell : _spells)
    {
        if (spell.spellId == spellId)
            return &spell;
    }
    return nullptr;
}

}} // namespace bot::ai
} // namespace Playerbot
