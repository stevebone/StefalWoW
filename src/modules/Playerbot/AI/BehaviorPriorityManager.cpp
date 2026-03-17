/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
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

#include "BehaviorPriorityManager.h"
#include "BotAI.h"
#include "BehaviorManager.h"
#include "Player.h"
#include "Group.h"
#include "Log.h"
#include <algorithm>
#include <sstream>

namespace Playerbot {

BehaviorPriorityManager::BehaviorPriorityManager(BotAI* ai)
    : m_ai(ai)
    , m_activePriority(BehaviorPriority::SOLO)
    , m_lastSelectedStrategy(nullptr)
{
    // ========================================================================
    // COMPREHENSIVE MUTUAL EXCLUSION RULES - Task 2.7
    // ========================================================================

    // COMBAT EXCLUSIONS (Priority 100)
    // Combat requires exclusive control - nothing else can run simultaneously
    AddExclusionRule(BehaviorPriority::COMBAT, BehaviorPriority::FOLLOW);
    AddExclusionRule(BehaviorPriority::COMBAT, BehaviorPriority::GATHERING);
    AddExclusionRule(BehaviorPriority::COMBAT, BehaviorPriority::TRADING);
    AddExclusionRule(BehaviorPriority::COMBAT, BehaviorPriority::SOCIAL);
    AddExclusionRule(BehaviorPriority::COMBAT, BehaviorPriority::SOLO);
    // Note: Combat allows MOVEMENT (45) for combat positioning
    // Note: Combat allows CASTING (80) for combat abilities

    // FLEEING EXCLUSIONS (Priority 90)
    // Fleeing overrides everything except death - survival is paramount
    AddExclusionRule(BehaviorPriority::FLEEING, BehaviorPriority::COMBAT);
    AddExclusionRule(BehaviorPriority::FLEEING, BehaviorPriority::FOLLOW);
    AddExclusionRule(BehaviorPriority::FLEEING, BehaviorPriority::GATHERING);
    AddExclusionRule(BehaviorPriority::FLEEING, BehaviorPriority::TRADING);
    AddExclusionRule(BehaviorPriority::FLEEING, BehaviorPriority::SOCIAL);
    AddExclusionRule(BehaviorPriority::FLEEING, BehaviorPriority::SOLO);
    AddExclusionRule(BehaviorPriority::FLEEING, BehaviorPriority::CASTING);
    // Note: Fleeing allows MOVEMENT (45) for escape paths

    // CASTING EXCLUSIONS (Priority 80)
    // Casting blocks movement but allows standing still
    AddExclusionRule(BehaviorPriority::CASTING, BehaviorPriority::MOVEMENT);
    AddExclusionRule(BehaviorPriority::CASTING, BehaviorPriority::FOLLOW);
    AddExclusionRule(BehaviorPriority::CASTING, BehaviorPriority::GATHERING);
    // Note: Casting is compatible with COMBAT (for combat spells)

    // FOLLOW EXCLUSIONS (Priority 50)
    // Follow behavior disabled during combat, casting, and fleeing
    // (Already covered by higher priority exclusions above)

    // MOVEMENT EXCLUSIONS (Priority 45)
    // Movement conflicts with activities requiring stationary position
    AddExclusionRule(BehaviorPriority::MOVEMENT, BehaviorPriority::TRADING);
    AddExclusionRule(BehaviorPriority::MOVEMENT, BehaviorPriority::SOCIAL);
    // Note: Movement is compatible with COMBAT (chase target)
    // Note: Movement is compatible with FOLLOW (move to leader)
    // Note: Movement is compatible with GATHERING (move to node)

    // GATHERING EXCLUSIONS (Priority 40)
    // Gathering is a low-priority activity interrupted by almost everything
    // (Already covered by higher priority exclusions above)
    AddExclusionRule(BehaviorPriority::GATHERING, BehaviorPriority::FOLLOW);
    AddExclusionRule(BehaviorPriority::GATHERING, BehaviorPriority::SOCIAL);

    // TRADING EXCLUSIONS (Priority 30)
    // Trading requires stationary position and focus
    AddExclusionRule(BehaviorPriority::TRADING, BehaviorPriority::FOLLOW);
    AddExclusionRule(BehaviorPriority::TRADING, BehaviorPriority::SOCIAL);

    // SOCIAL EXCLUSIONS (Priority 20)
    // Social behaviors are lowest priority activities
    // (Already covered by higher priority exclusions above)

    // DEAD STATE EXCLUSIONS (Priority 0)
    // Dead bots can't do anything
    AddExclusionRule(BehaviorPriority::DEAD, BehaviorPriority::COMBAT);
    AddExclusionRule(BehaviorPriority::DEAD, BehaviorPriority::FOLLOW);
    AddExclusionRule(BehaviorPriority::DEAD, BehaviorPriority::MOVEMENT);
    AddExclusionRule(BehaviorPriority::DEAD, BehaviorPriority::GATHERING);
    AddExclusionRule(BehaviorPriority::DEAD, BehaviorPriority::TRADING);
    AddExclusionRule(BehaviorPriority::DEAD, BehaviorPriority::SOCIAL);
    AddExclusionRule(BehaviorPriority::DEAD, BehaviorPriority::SOLO);
    AddExclusionRule(BehaviorPriority::DEAD, BehaviorPriority::CASTING);
    AddExclusionRule(BehaviorPriority::DEAD, BehaviorPriority::FLEEING);

    // ERROR STATE EXCLUSIONS (Priority 5)
    // Error state prevents all other behaviors
    AddExclusionRule(BehaviorPriority::ERROR_PRIORITY, BehaviorPriority::COMBAT);
    AddExclusionRule(BehaviorPriority::ERROR_PRIORITY, BehaviorPriority::FOLLOW);
    AddExclusionRule(BehaviorPriority::ERROR_PRIORITY, BehaviorPriority::MOVEMENT);
    AddExclusionRule(BehaviorPriority::ERROR_PRIORITY, BehaviorPriority::GATHERING);
    AddExclusionRule(BehaviorPriority::ERROR_PRIORITY, BehaviorPriority::TRADING);
    AddExclusionRule(BehaviorPriority::ERROR_PRIORITY, BehaviorPriority::SOCIAL);
    AddExclusionRule(BehaviorPriority::ERROR_PRIORITY, BehaviorPriority::SOLO);
    AddExclusionRule(BehaviorPriority::ERROR_PRIORITY, BehaviorPriority::CASTING);
    AddExclusionRule(BehaviorPriority::ERROR_PRIORITY, BehaviorPriority::FLEEING);

    // CRITICAL: Do NOT access m_ai->GetBot()->GetName() in constructor!
    // Bot may not be fully in world yet, and Player::m_name is not initialized.
}

BehaviorPriorityManager::~BehaviorPriorityManager()
{
    // CRITICAL: Do NOT access bot->GetName() during destructor!
    // During bot destruction, the Player's internal string data may already be
    // freed even if IsInWorld() returns true. This causes ACCESS_VIOLATION in
    // the string formatting code.
    //
    // If logging is needed, use GUID counter instead:
    // Player* bot = m_ai ? m_ai->GetBot() : nullptr;
    // if (bot)
    //     TC_LOG_DEBUG("module.playerbot.priority",
    //         "BehaviorPriorityManager destroyed for bot {}", bot->GetGUID().GetCounter());
}

// ========================================================================
// STRATEGY REGISTRATION
// ========================================================================

void BehaviorPriorityManager::RegisterStrategy(
    Strategy* strategy,
    BehaviorPriority priority,
    bool exclusive)
{

    BehaviorMetadata metadata;
    metadata.strategy = strategy;
    metadata.priority = priority;
    metadata.exclusive = exclusive;
    metadata.allowsLowerPriority = !exclusive;

    // Add to appropriate priority bucket
    m_strategies[priority].push_back(metadata);

    TC_LOG_DEBUG("module.playerbot.priority",
        "Registered strategy {} with priority {} (exclusive: {})",
        strategy->GetName(),
        static_cast<int>(priority),
        exclusive);
}

void BehaviorPriorityManager::UnregisterStrategy(Strategy* strategy)
{
    if (!strategy)
        return;

    // Remove from all priority buckets
    for (auto& [priority, strategies] : m_strategies)
    {
        auto it = ::std::remove_if(strategies.begin(), strategies.end(),
            [strategy](const BehaviorMetadata& meta) {
                return meta.strategy == strategy;
            });

        if (it != strategies.end())
        {
            TC_LOG_DEBUG("module.playerbot.priority",
                "Unregistered strategy {} from priority {}",
                strategy->GetName(),
                static_cast<int>(priority));
            strategies.erase(it, strategies.end());
        }
    }

    // Clear last selected if it was this strategy
    if (m_lastSelectedStrategy == strategy)
        m_lastSelectedStrategy = nullptr;
}

void BehaviorPriorityManager::AddExclusionRule(BehaviorPriority a, BehaviorPriority b)
{
    // Add bidirectional exclusion
    m_exclusionRules[a].insert(b);
    m_exclusionRules[b].insert(a);

    TC_LOG_DEBUG("module.playerbot.priority",
        "Added exclusion rule: {} <-> {}",
        static_cast<int>(a),
        static_cast<int>(b));
}

// ========================================================================
// STRATEGY SELECTION
// ========================================================================

Strategy* BehaviorPriorityManager::SelectActiveBehavior(
    ::std::vector<Strategy*>& activeStrategies)
{
    // CRITICAL FIX (2024-12-16): Changed TC_LOG_ERROR to TC_LOG_TRACE
    // These are debug statements, not errors. Excessive ERROR logging from
    // worker threads was causing heap corruption in WriteWinConsole due to
    // string formatting race conditions with Strategy::GetName() references.
    //
    // Also cache strategy names as copies (not references) before logging
    // to prevent use-after-free if strategy is deleted during log formatting.

    TC_LOG_TRACE("module.playerbot.priority", "SelectActiveBehavior ENTRY: {} active strategies",
                 activeStrategies.size());

    if (activeStrategies.empty())
    {
        TC_LOG_TRACE("module.playerbot.priority", "SelectActiveBehavior: No active strategies, returning nullptr");
        m_lastSelectedStrategy = nullptr;
        return nullptr;
    }

    // Build list of active priorities and their strategies
    ::std::vector<::std::pair<BehaviorPriority, Strategy*>> prioritizedStrategies;

    for (Strategy* strategy : activeStrategies)
    {
        if (!strategy)
            continue;

        const BehaviorMetadata* meta = FindMetadata(strategy);
        if (!meta)
        {
            // Strategy not registered, default to SOLO priority
            // Cache name as copy to prevent use-after-free
            ::std::string stratName = strategy->GetName();
            TC_LOG_TRACE("module.playerbot.priority",
                "SelectActiveBehavior: Strategy '{}' NOT registered, using SOLO priority",
                stratName);
            prioritizedStrategies.emplace_back(BehaviorPriority::SOLO, strategy);
        }
        else
        {
            // Check if this is a BehaviorManager and if it's enabled
            if (auto* behaviorMgr = dynamic_cast<BehaviorManager*>(strategy))
            {
                if (!behaviorMgr->IsEnabled())
                    continue;
            }

            prioritizedStrategies.emplace_back(meta->priority, strategy);
        }
    }

    if (prioritizedStrategies.empty())
    {
        m_lastSelectedStrategy = nullptr;
        return nullptr;
    }

    // Sort by priority (descending)
    ::std::sort(prioritizedStrategies.begin(), prioritizedStrategies.end(),
        [](const auto& a, const auto& b) {
            return a.first > b.first;
        });

    // Track which priorities are actually viable (non-zero relevance)
    // This prevents strategies with 0.0f relevance from blocking lower-priority strategies
    ::std::vector<BehaviorPriority> viablePriorities;

    for (const auto& [priority, strategy] : prioritizedStrategies)
    {
        if (!strategy)
            continue;

        // Check if strategy's IsActive method returns true
        if (!strategy->IsActive(m_ai))
            continue;

        // CRITICAL FIX: Check strategy relevance BEFORE exclusion check
        // This prevents strategies with 0.0f relevance from blocking lower-priority strategies
        float relevance = strategy->GetRelevance(m_ai);

        if (relevance <= 0.0f)
            continue;

        // Strategy has viable relevance - add to viable list for exclusion checking
        viablePriorities.push_back(priority);

        // Check if blocked by OTHER VIABLE priorities
        if (IsBlockedByExclusion(priority, viablePriorities))
            continue;

        // This is our winner - log only if changed
        if (m_lastSelectedStrategy != strategy)
        {
            // Cache names as copies for thread-safe logging
            ::std::string newStratName = strategy->GetName();
            ::std::string oldStratName = m_lastSelectedStrategy ?
                m_lastSelectedStrategy->GetName() : "none";

            TC_LOG_DEBUG("module.playerbot.priority",
                "Strategy changed: '{}' -> '{}' (priority {}, relevance {:.1f})",
                oldStratName,
                newStratName,
                static_cast<int>(priority),
                relevance);

            m_lastSelectedStrategy = strategy;
            m_activePriority = priority;
        }

        return strategy;
    }

    // No strategy selected
    if (m_lastSelectedStrategy != nullptr)
    {
        TC_LOG_DEBUG("module.playerbot.priority",
            "No strategy selected (was: {})",
            m_lastSelectedStrategy->GetName());

        m_lastSelectedStrategy = nullptr;
        m_activePriority = BehaviorPriority::SOLO;
    }

    return nullptr;
}

::std::vector<Strategy*> BehaviorPriorityManager::GetPrioritizedStrategies() const
{
    ::std::vector<Strategy*> result;

    // Iterate through priorities in descending order
    for (auto it = m_strategies.rbegin(); it != m_strategies.rend(); ++it)
    {
        for (const BehaviorMetadata& meta : it->second)
        {
            if (meta.strategy)
                result.push_back(meta.strategy);
        }
    }

    return result;
}

bool BehaviorPriorityManager::CanCoexist(Strategy* a, Strategy* b) const
{
    if (!a || !b || a == b)
        return true;

    const BehaviorMetadata* metaA = FindMetadata(a);
    const BehaviorMetadata* metaB = FindMetadata(b);

    if (!metaA || !metaB)
        return true; // Unknown strategies can coexist

    // Check exclusion rules
    auto itA = m_exclusionRules.find(metaA->priority);
    if (itA != m_exclusionRules.end())
    {
        if (itA->second.count(metaB->priority) > 0)
            return false; // These priorities are mutually exclusive
    }

    // Check exclusive flags
    if (metaA->exclusive || metaB->exclusive)
        return false;

    return true;
}

bool BehaviorPriorityManager::IsAllowedToRun(Strategy* strategy) const
{
    if (!strategy)
        return false;

    const BehaviorMetadata* meta = FindMetadata(strategy);
    if (!meta)
        return true; // Unknown strategies are allowed by default

    // Check if any higher priority is active and exclusive
    for (const auto& [priority, strategies] : m_strategies)
    {
        if (priority <= meta->priority)
            continue; // Only check higher priorities
    for (const BehaviorMetadata& otherMeta : strategies)
        {
            if (!otherMeta.strategy || !otherMeta.strategy->IsActive(const_cast<BotAI*>(m_ai)))
                continue;

            // If higher priority is exclusive, block this one
    if (otherMeta.exclusive)
                return false;

            // Check exclusion rules
    if (!CanCoexist(strategy, otherMeta.strategy))
                return false;
        }
    }

    return true;
}

// ========================================================================
// CONTEXT & STATE
// ========================================================================

void BehaviorPriorityManager::UpdateContext()
{
    if (!m_ai || !m_ai->GetBot())
        return;

    Player* bot = m_ai->GetBot();
    // Check if bot is dead
    if (!bot->IsAlive())
    {
        if (m_activePriority != BehaviorPriority::DEAD)
        {
            TC_LOG_DEBUG("module.playerbot.priority",
                "Bot {} entering DEAD priority",
                bot->GetName());
            m_activePriority = BehaviorPriority::DEAD;
        }
        return;
    }

    // CRITICAL FIX: Check critically low health REGARDLESS of combat state
    // Bots at <20% health should always prioritize survival (flee/heal/rest)
    // This prevents bots at 0.6% health from running SOLO strategies like RestStrategy
    float healthPct = bot->GetHealthPct();
    if (healthPct < 20.0f)
    {
        if (m_activePriority != BehaviorPriority::FLEEING)
        {
            TC_LOG_DEBUG("module.playerbot.priority",
                "Bot {} entering FLEEING priority (critical health: {:.1f}%)",
                bot->GetName(),
                healthPct);
            m_activePriority = BehaviorPriority::FLEEING;
        }
        return; // Don't process other priorities when critically low health
    }

    // Check combat state (only if not critically low health)
    if (bot->IsInCombat())
    {
        if (m_activePriority != BehaviorPriority::COMBAT)
        {
            TC_LOG_DEBUG("module.playerbot.priority",
                "Bot {} entering COMBAT priority",
                bot->GetName());
            m_activePriority = BehaviorPriority::COMBAT;
        }
    }
    // Check if casting
    else if (bot->IsNonMeleeSpellCast(false))
    {
        if (m_activePriority != BehaviorPriority::CASTING)
        {
            TC_LOG_DEBUG("module.playerbot.priority",
                "Bot {} entering CASTING priority",
                bot->GetName());
            m_activePriority = BehaviorPriority::CASTING;
        }
    }
    // Check if following (has group and not leader)
    else if (bot->GetGroup() && !bot->GetGroup()->IsLeader(bot->GetGUID()))
    {
        if (m_activePriority != BehaviorPriority::FOLLOW &&
            m_activePriority != BehaviorPriority::GATHERING &&
            m_activePriority != BehaviorPriority::TRADING)
        {
            TC_LOG_DEBUG("module.playerbot.priority",
                "Bot {} entering FOLLOW priority",
                bot->GetName());
            m_activePriority = BehaviorPriority::FOLLOW;
        }
    }
    // Default to solo
    else
    {
        if (m_activePriority == BehaviorPriority::COMBAT ||
            m_activePriority == BehaviorPriority::FLEEING ||
            m_activePriority == BehaviorPriority::CASTING ||
            m_activePriority == BehaviorPriority::DEAD)
        {
            TC_LOG_DEBUG("module.playerbot.priority",
                "Bot {} returning to SOLO priority",
                bot->GetName());
            m_activePriority = BehaviorPriority::SOLO;
        }
    }
}

BehaviorPriority BehaviorPriorityManager::GetActivePriority() const
{
    return m_activePriority;
}

bool BehaviorPriorityManager::IsPriorityActive(BehaviorPriority priority) const
{
    return m_activePriority == priority;
}

// ========================================================================
// DIAGNOSTICS
// ========================================================================

void BehaviorPriorityManager::DumpPriorityState() const
{
    ::std::stringstream ss;
    ss << "BehaviorPriorityManager State:\n";
    ss << "  Active Priority: " << ToString(m_activePriority) << " ("
       << static_cast<int>(m_activePriority) << ")\n";

    if (m_lastSelectedStrategy)
    {
        ss << "  Last Selected: " << m_lastSelectedStrategy->GetName() << "\n";
    }

    ss << "  Registered Strategies:\n";
    for (const auto& [priority, strategies] : m_strategies)
    {
        if (strategies.empty())
            continue;

        ss << "    Priority " << ToString(priority) << " ("
           << static_cast<int>(priority) << "):\n";

        for (const BehaviorMetadata& meta : strategies)
        {
            if (meta.strategy)
            {
                ss << "      - " << meta.strategy->GetName()
                   << " (exclusive: " << meta.exclusive
                   << ", active: " << meta.strategy->IsActive(const_cast<BotAI*>(m_ai))
                   << ")\n";
            }
        }
    }

    ss << "  Exclusion Rules:\n";
    for (const auto& [priority, conflicts] : m_exclusionRules)
    {
        if (conflicts.empty())
            continue;

        ss << "    " << ToString(priority) << " excludes: ";
        bool first = true;
        for (BehaviorPriority conflict : conflicts)
        {
            if (!first) ss << ", ";
            ss << ToString(conflict);
            first = false;
        }
        ss << "\n";
    }

    TC_LOG_INFO("module.playerbot.priority", "{}", ss.str());
}

::std::set<BehaviorPriority> BehaviorPriorityManager::GetConflicts(BehaviorPriority priority) const
{
    auto it = m_exclusionRules.find(priority);
    if (it != m_exclusionRules.end())
        return it->second;
    return {};
}

// ========================================================================
// PRIVATE HELPERS
// ========================================================================

bool BehaviorPriorityManager::IsBlockedByExclusion(
    BehaviorPriority priority,
    const ::std::vector<BehaviorPriority>& activePriorities) const
{
    auto it = m_exclusionRules.find(priority);
    if (it == m_exclusionRules.end())
    {
        return false; // No exclusion rules for this priority
    }

    const ::std::set<BehaviorPriority>& conflicts = it->second;

    // Check if any active priority conflicts
    for (BehaviorPriority activePriority : activePriorities)
    {
        if (activePriority == priority)
            continue; // Don't check self

        // If this priority conflicts with an active one AND
        // the active one has higher priority, we're blocked
    if (conflicts.count(activePriority) && activePriority > priority)
        {
            TC_LOG_TRACE("module.playerbot.priority",
                "Priority {} blocked by higher priority {}",
                static_cast<int>(priority),
                static_cast<int>(activePriority));
            return true;
        }
    }

    return false;
}

BehaviorMetadata* BehaviorPriorityManager::FindMetadata(Strategy* strategy)
{
    if (!strategy)
        return nullptr;

    for (auto& [priority, strategies] : m_strategies)
    {
        for (auto& meta : strategies)
        {
            if (meta.strategy == strategy)
                return &meta;
        }
    }

    return nullptr;
}

const BehaviorMetadata* BehaviorPriorityManager::FindMetadata(Strategy* strategy) const
{
    if (!strategy)
        return nullptr;

    for (const auto& [priority, strategies] : m_strategies)
    {
        for (const auto& meta : strategies)
        {
            if (meta.strategy == strategy)
                return &meta;
        }
    }

    return nullptr;
}

// ========================================================================
// HELPER FUNCTIONS
// ========================================================================

::std::string_view ToString(BehaviorPriority priority)
{
    switch (priority)
    {
        case BehaviorPriority::DEAD:      return "DEAD";
        case BehaviorPriority::ERROR_PRIORITY:     return "ERROR";
        case BehaviorPriority::SOLO:      return "SOLO";
        case BehaviorPriority::SOCIAL:    return "SOCIAL";
        case BehaviorPriority::TRADING:   return "TRADING";
        case BehaviorPriority::GATHERING: return "GATHERING";
        case BehaviorPriority::MOVEMENT:  return "MOVEMENT";
        case BehaviorPriority::FOLLOW:    return "FOLLOW";
        case BehaviorPriority::CASTING:   return "CASTING";
        case BehaviorPriority::FLEEING:   return "FLEEING";
        case BehaviorPriority::COMBAT:    return "COMBAT";
        default:                          return "UNKNOWN";
    }
}

} // namespace Playerbot