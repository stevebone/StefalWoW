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

/**
 * @file BotAI_UtilityAI.cpp
 * @brief Utility AI integration for BotAI - Phase 1 Week 2
 *
 * This file contains the Utility AI decision system integration for BotAI.
 * Part of the Hybrid AI Decision System (Utility AI + Behavior Trees).
 */

#include "BotAI.h"
#include "Utility/UtilitySystem.h"
#include "Utility/UtilityContextBuilder.h"
#include "Utility/Evaluators/CombatEvaluators.h"
#include "Player.h"
#include "Log.h"
#include "Timer.h"

namespace Playerbot
{

// ============================================================================
// UTILITY AI INITIALIZATION
// ============================================================================

void BotAI::InitializeUtilityAI()
{
    TC_LOG_DEBUG("playerbot.utility", "Initializing Utility AI for bot {}", _bot->GetName());

    _utilityAI = ::std::make_unique<UtilityAI>();

    // ========================================================================
    // BEHAVIOR 1: COMBAT
    // ========================================================================
    auto combatBehavior = ::std::make_shared<UtilityBehavior>("Combat");
    combatBehavior->AddEvaluator(::std::make_shared<CombatEngageEvaluator>());
    combatBehavior->AddEvaluator(::std::make_shared<DefensiveCooldownEvaluator>());
    _utilityAI->AddBehavior(combatBehavior);

    // ========================================================================
    // BEHAVIOR 2: HEALING (Healer role only)
    // ========================================================================
    auto healingBehavior = ::std::make_shared<UtilityBehavior>("Healing");
    healingBehavior->AddEvaluator(::std::make_shared<HealAllyEvaluator>());
    _utilityAI->AddBehavior(healingBehavior);

    // ========================================================================
    // BEHAVIOR 3: TANKING (Tank role only)
    // ========================================================================
    auto tankingBehavior = ::std::make_shared<UtilityBehavior>("Tanking");
    tankingBehavior->AddEvaluator(::std::make_shared<TankThreatEvaluator>());
    tankingBehavior->AddEvaluator(::std::make_shared<DefensiveCooldownEvaluator>());
    _utilityAI->AddBehavior(tankingBehavior);

    // ========================================================================
    // BEHAVIOR 4: FLEE (All roles - survival)
    // ========================================================================
    auto fleeBehavior = ::std::make_shared<UtilityBehavior>("Flee");
    fleeBehavior->AddEvaluator(::std::make_shared<FleeEvaluator>());
    _utilityAI->AddBehavior(fleeBehavior);

    // ========================================================================
    // BEHAVIOR 5: MANA REGENERATION (Mana users only)
    // ========================================================================
    auto manaRegenBehavior = ::std::make_shared<UtilityBehavior>("ManaRegeneration");
    manaRegenBehavior->AddEvaluator(::std::make_shared<ManaRegenerationEvaluator>());
    _utilityAI->AddBehavior(manaRegenBehavior);

    // ========================================================================
    // BEHAVIOR 6: AOE DAMAGE (DPS role - multi-target)
    // ========================================================================
    auto aoeBehavior = ::std::make_shared<UtilityBehavior>("AoEDamage");
    aoeBehavior->AddEvaluator(::std::make_shared<AoEDamageEvaluator>());
    _utilityAI->AddBehavior(aoeBehavior);

    // ========================================================================
    // BEHAVIOR 7: DISPEL (Healer/Support only)
    // ========================================================================
    auto dispelBehavior = ::std::make_shared<UtilityBehavior>("Dispel");
    dispelBehavior->AddEvaluator(::std::make_shared<DispelEvaluator>());
    _utilityAI->AddBehavior(dispelBehavior);

    // Initialize state
    _activeUtilityBehavior = nullptr;
    _lastUtilityUpdate = 0;

    TC_LOG_INFO("playerbot.utility", "Utility AI initialized for bot {} with {} behaviors",
        _bot->GetName(), _utilityAI->GetBehaviors().size());
}

// ============================================================================
// UTILITY AI UPDATE
// ============================================================================

void BotAI::UpdateUtilityDecision(uint32 diff)
{

    // Throttle updates to every 500ms
    _lastUtilityUpdate += diff;
    if (_lastUtilityUpdate < 500)
        return;

    _lastUtilityUpdate = 0;

    // Build current context from game world state
    _lastUtilityContext = UtilityContextBuilder::Build(this, nullptr);

    // Select best behavior based on context
    UtilityBehavior* newBehavior = _utilityAI->SelectBehavior(_lastUtilityContext);

    // Log behavior transitions
    if (newBehavior != _activeUtilityBehavior)
    {
        ::std::string oldName = _activeUtilityBehavior ? _activeUtilityBehavior->GetName() : "None";
        ::std::string newName = newBehavior ? newBehavior->GetName() : "None";

        TC_LOG_DEBUG("playerbot.utility", "Bot {} utility behavior transition: {} -> {} (score: {:.3f})",
            _bot->GetName(),
            oldName,
            newName,
            newBehavior ? newBehavior->GetCachedScore() : 0.0f);

        // Detailed context logging at debug level
        TC_LOG_TRACE("playerbot.utility", "  Context: health={:.2f} mana={:.2f} combat={} enemies={} role={}",
            _lastUtilityContext.healthPercent,
            _lastUtilityContext.manaPercent,
            _lastUtilityContext.inCombat ? "yes" : "no",
            _lastUtilityContext.enemiesInRange,
            static_cast<int>(_lastUtilityContext.role));
    }

    _activeUtilityBehavior = newBehavior;

    // Performance tracking: Log top 3 behaviors for analysis
    if (TC_LOG_WILL_LOG("playerbot.utility.detailed", LOG_LEVEL_TRACE))
    {
        auto ranked = _utilityAI->GetRankedBehaviors(_lastUtilityContext);
        size_t count = ::std::min<size_t>(3, ranked.size());

        TC_LOG_TRACE("playerbot.utility.detailed", "Bot {} top {} behaviors:",
            _bot->GetName(), count);

        for (size_t i = 0; i < count; ++i)
        {
            auto const& [behavior, score] = ranked[i];
            TC_LOG_TRACE("playerbot.utility.detailed", "  {}. {} - score: {:.4f}",
                i + 1, behavior->GetName(), score);
        }
    }
}

} // namespace Playerbot
