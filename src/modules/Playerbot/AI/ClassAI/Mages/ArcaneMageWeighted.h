/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
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

#ifndef PLAYERBOT_ARCANEMAGEWEIGHTED_H
#define PLAYERBOT_ARCANEMAGEWEIGHTED_H

#include "ArcaneMage.h"
#include "../../Common/ActionScoringEngine.h"
#include "../../Common/CombatContextDetector.h"
#include "../../BehaviorTree/BehaviorTree.h"
#include "Config/PlayerbotConfig.h"

namespace Playerbot
{

/**
 * @class ArcaneMageWeighted
 * @brief Demonstration of Arcane Mage with utility-based weighting system integration
 *
 * This class extends ArcaneMageRefactored to demonstrate the weighting system integration pattern.
 * It serves as a reference implementation showing how to integrate ActionScoringEngine with ClassAI specs.
 *
 * Key Features:
 * - Automatic context detection (solo/group/dungeon/raid/PvP)
 * - Multi-criteria action scoring (survival, damage, resource, positioning, strategic)
 * - Intelligent cooldown alignment based on context and role
 * - Human-like decision-making patterns
 *
 * Integration Pattern (apply to other specs):
 * 1. Add ActionScoringEngine member
 * 2. Initialize with detected context in constructor
 * 3. Create scoring functions for each ability
 * 4. Replace rotation logic with scored action selection
 * 5. Update context dynamically when group/instance state changes
 */
class ArcaneMageWeighted : public ArcaneMageRefactored
{
public:
    using Base = ArcaneMageRefactored;

    explicit ArcaneMageWeighted(Player* bot)
        : ArcaneMageRefactored(bot)
        , _scoringEngine(bot::ai::BotRole::RANGED_DPS, bot::ai::CombatContext::SOLO)
        , _lastContextUpdate(0)
        , _contextUpdateInterval(5000) // Update context every 5 seconds
    {
        // Enable debug logging if configured
        bool debugLogging = sPlayerbotConfig->GetBool("Playerbot.AI.Weighting.LogScoring", false);
        _scoringEngine.EnableDebugLogging(debugLogging);

        TC_LOG_DEBUG("playerbot", "ArcaneMageWeighted initialized for bot {} with weighting system", bot->GetGUID().GetCounter());
    }

    /**
     * @brief Main rotation update with weighting system
     * Overrides base rotation to use utility-based action scoring
     */
    void UpdateRotation(::Unit* target) override
    {
        if (!target || !this->GetBot())
            return;

        // Update context periodically
        UpdateCombatContext();

        // Update arcane state (charges, procs, cooldowns)
        UpdateArcaneState();

        // Score all available actions and execute best one
        ExecuteWeightedRotation(target);
    }

private:
    /**
     * @brief Update combat context if needed
     * Detects current combat situation and updates scoring engine
     */
    void UpdateCombatContext()
    {
        uint32 currentTime = GameTime::GetGameTimeMS();
        if (currentTime - _lastContextUpdate < _contextUpdateInterval)
            return; // Not time to update yet

        _lastContextUpdate = currentTime;

        // Detect new context
        bot::ai::CombatContext newContext = bot::ai::CombatContextDetector::DetectContext(this->GetBot());

        // Update scoring engine if context changed
        if (newContext != _scoringEngine.GetContext())
        {
            _scoringEngine.SetContext(newContext);

            TC_LOG_DEBUG("playerbot", "ArcaneMageWeighted: Context changed to {}",
                bot::ai::ActionScoringEngine::GetContextName(newContext));
        }
    }

    /**
     * @brief Execute rotation using weighted action scoring
     * This is the core of the weighting system integration
     */
    void ExecuteWeightedRotation(::Unit* target)
    {
        Player* bot = this->GetBot();
        if (!bot)
            return;

        // Gather current state for scoring
        uint32 charges = _chargeTracker.GetCharges();
        uint32 manaPercent = bot->GetPowerPct(POWER_MANA);
        float healthPercent = bot->GetHealthPct();
        uint32 enemyCount = this->GetEnemiesInRange(40.0f);
        bool inCombat = bot->IsInCombat();

        // Build list of available actions
        ::std::vector<uint32> availableActions;

        // Always consider these spells if ready
        if (this->IsSpellReady(ARCANE_BLAST))
            availableActions.push_back(ARCANE_BLAST);

        if (this->IsSpellReady(ARCANE_BARRAGE))
            availableActions.push_back(ARCANE_BARRAGE);

        if (_clearcastingTracker.IsActive() && this->IsSpellReady(ARCANE_MISSILES))
            availableActions.push_back(ARCANE_MISSILES);

        if (this->IsSpellReady(ARCANE_SURGE) && !_arcaneSurgeActive)
            availableActions.push_back(ARCANE_SURGE);

        if (this->IsSpellReady(TOUCH_OF_MAGE) && bot->HasSpell(TOUCH_OF_MAGE))
            availableActions.push_back(TOUCH_OF_MAGE);

        if (this->IsSpellReady(PRESENCE_OF_MIND))
            availableActions.push_back(PRESENCE_OF_MIND);

        if (enemyCount >= 3)
        {
            if (this->IsSpellReady(ARCANE_ORB) && bot->HasSpell(ARCANE_ORB))
                availableActions.push_back(ARCANE_ORB);

            if (this->IsSpellReady(ARCANE_EXPLOSION))
                availableActions.push_back(ARCANE_EXPLOSION);
        }

        // Defensive abilities
        if (healthPercent < 50.0f)
        {
            if (this->IsSpellReady(ICE_BLOCK))
                availableActions.push_back(ICE_BLOCK);

            if (this->IsSpellReady(MIRROR_IMAGE))
                availableActions.push_back(MIRROR_IMAGE);

            if (this->IsSpellReady(SHIFTING_POWER))
                availableActions.push_back(SHIFTING_POWER);
        }

        // Resource recovery
        if (manaPercent < 30 && this->IsSpellReady(EVOCATION))
            availableActions.push_back(EVOCATION);

        if (availableActions.empty())
            return; // Nothing to do

        // Score all available actions
        ::std::vector<bot::ai::ActionScore> scores = _scoringEngine.ScoreActions(
            availableActions,
            [this, charges, manaPercent, healthPercent, enemyCount, bot, target]
            (bot::ai::ScoringCategory category, uint32 actionId) -> float
            {
                return ScoreAction(actionId, category, charges, manaPercent, healthPercent, enemyCount);
            }
        );

        // Get top actions (for logging if debug enabled)
        if (_scoringEngine.IsDebugLoggingEnabled())
        {
            uint32 topCount = sPlayerbotConfig->GetInt("Playerbot.AI.Weighting.LogTopActions", 3);
            ::std::vector<uint32> topActions = _scoringEngine.GetTopActions(scores, topCount);

            if (!topActions.empty())
            {
                ::std::ostringstream oss;
                oss << "Top " << topActions.size() << " scored actions: ";
                for (size_t i = 0; i < topActions.size(); ++i)
                {
                    auto scoreIt = ::std::find_if(scores.begin(), scores.end(),
                        [id = topActions[i]](const bot::ai::ActionScore& s) { return s.actionId == id; });

                    if (scoreIt != scores.end())
                        oss << topActions[i] << " (" << ::std::fixed << ::std::setprecision(1) << scoreIt->totalScore << ")";

                    if (i < topActions.size() - 1)
                        oss << ", ";
                }
                TC_LOG_DEBUG("playerbot.weighting", "{}", oss.str());
            }
        }

        // Execute best action
        uint32 bestAction = _scoringEngine.GetBestAction(scores);
        if (bestAction != 0)
        {
            ExecuteAction(bestAction, target);
        }
    }

    /**
     * @brief Score an individual action across all categories
     * This is where spec-specific knowledge is encoded
     *
     * @param actionId Spell ID to score
     * @param category Scoring category (Survival, Damage, Resource, etc.)
     * @param charges Current Arcane Charges
     * @param manaPercent Current mana percentage
     * @param healthPercent Current health percentage
     * @param enemyCount Number of nearby enemies
     * @return Score value 0.0-1.0 for this category
     */
    float ScoreAction(
        uint32 actionId,
        bot::ai::ScoringCategory category,
        uint32 charges,
        uint32 manaPercent,
        float healthPercent,
        uint32 enemyCount) const
    {
        using bot::ai::ScoringCategory;

        switch (category)
        {
            case ScoringCategory::SURVIVAL:
                return ScoreSurvival(actionId, healthPercent);

            case ScoringCategory::DAMAGE_OPTIMIZATION:
                return ScoreDamage(actionId, charges, manaPercent, enemyCount);

            case ScoringCategory::RESOURCE_EFFICIENCY:
                return ScoreResource(actionId, manaPercent, charges);

            case ScoringCategory::STRATEGIC_VALUE:
                return ScoreStrategic(actionId, charges, manaPercent);

            case ScoringCategory::GROUP_PROTECTION:
            case ScoringCategory::POSITIONING_MECHANICS:
                // Arcane Mage is ranged DPS - minimal group protection and positioning concerns
                return 0.0f;

            default:
                return 0.0f;
        }
    }

    /**
     * @brief Score survival value of an action
     * High scores for defensive abilities when health is low
     */
    float ScoreSurvival(uint32 actionId, float healthPercent) const
    {
        float urgency = (100.0f - healthPercent) / 100.0f; // 0.0 at 100% HP, 1.0 at 0% HP

        switch (actionId)
        {
            case ICE_BLOCK:
                // Ice Block = immunity, highest survival value when critically low
                if (healthPercent < 20.0f)
                    return 1.0f; // Maximum urgency
                else if (healthPercent < 40.0f)
                    return 0.6f;
                else
                    return 0.0f;

            case MIRROR_IMAGE:
                // Mirror Image = defensive decoy
                if (healthPercent < 40.0f)
                    return 0.8f * urgency;
                else
                    return 0.0f;

            case SHIFTING_POWER:
                // Shifting Power = CD reset, can help survive
                if (healthPercent < 50.0f)
                    return 0.5f * urgency;
                else
                    return 0.0f;

            case EVOCATION:
                // Evocation = channeled, vulnerable but recovers mana
                if (healthPercent > 70.0f) // Safe to channel
                    return 0.3f;
                else
                    return 0.0f; // Too dangerous

            default:
                return 0.0f; // No survival value
        }
    }

    /**
     * @brief Score damage optimization value of an action
     * Considers charge state, cooldowns, enemy count, procs
     */
    float ScoreDamage(uint32 actionId, uint32 charges, uint32 manaPercent, uint32 enemyCount) const
    {
        bool isAoE = (enemyCount >= 3);

        switch (actionId)
        {
            case ARCANE_SURGE:
            {
                // Major DPS cooldown - highest value at 4 charges with good mana
                float chargeValue = static_cast<float>(charges) / 4.0f; // 0.0-1.0
                float manaValue = manaPercent >= 70 ? 1.0f : 0.3f;
                float contextValue = 0.7f; // Base value

                // Higher value in AoE
                if (isAoE)
                    contextValue = 0.9f;

                // Context bonus (raid boss > dungeon boss > trash/solo)
                bot::ai::CombatContext context = _scoringEngine.GetContext();
                if (context == bot::ai::CombatContext::RAID_HEROIC || context == bot::ai::CombatContext::RAID_NORMAL)
                    contextValue = 1.0f;
                else if (context == bot::ai::CombatContext::DUNGEON_BOSS)
                    contextValue = 0.9f;

                return chargeValue * manaValue * contextValue;
            }

            case TOUCH_OF_MAGE:
            {
                // Damage amplification debuff - best at 4 charges
                if (charges >= 4)
                    return 0.8f;
                else if (charges >= 3)
                    return 0.5f;
                else
                    return 0.0f;
            }

            case ARCANE_MISSILES:
            {
                // Free cast with Clearcasting - always good value
                if (_clearcastingTracker.IsActive())
                    return 0.7f; // High value (free cast, no mana cost)
                else
                    return 0.0f; // Should not cast without proc
            }

            case ARCANE_BARRAGE:
            {
                // Spender - best at 4 charges, emergency at low mana
                if (charges >= 4)
                    return 0.8f; // High value at max charges
                else if (charges >= 2 && manaPercent < 30)
                    return 0.6f; // Emergency mana conservation
                else
                    return 0.2f; // Suboptimal but viable
            }

            case ARCANE_BLAST:
            {
                // Builder - constant value, higher when building charges
                if (charges < 4)
                    return 0.6f; // Good value when building
                else
                    return 0.3f; // Lower value at max charges (should spend)
            }

            case PRESENCE_OF_MIND:
            {
                // Instant cast buff - good for charge building
                if (charges < 4)
                    return 0.5f;
                else
                    return 0.2f;
            }

            case ARCANE_ORB:
            {
                // AoE builder
                if (isAoE && charges < 4)
                    return 0.8f; // Excellent in AoE
                else if (isAoE)
                    return 0.5f;
                else
                    return 0.2f; // Weak in single target
            }

            case ARCANE_EXPLOSION:
            {
                // AoE filler
                if (isAoE && enemyCount >= 5)
                    return 0.6f;
                else if (isAoE)
                    return 0.4f;
                else
                    return 0.0f; // Single target only
            }

            default:
                return 0.0f;
        }
    }

    /**
     * @brief Score resource efficiency value of an action
     * Considers mana cost, conservation, charge state
     */
    float ScoreResource(uint32 actionId, uint32 manaPercent, uint32 charges) const
    {
        switch (actionId)
        {
            case EVOCATION:
            {
                // Mana recovery - highest value when low mana
                if (manaPercent < 20)
                    return 1.0f; // Critical mana recovery needed
                else if (manaPercent < 40)
                    return 0.7f;
                else if (manaPercent < 60)
                    return 0.3f;
                else
                    return 0.0f;
            }

            case ARCANE_MISSILES:
            {
                // Free with Clearcasting - perfect resource efficiency
                if (_clearcastingTracker.IsActive())
                    return 1.0f; // Free cast = best efficiency
                else
                    return 0.0f;
            }

            case ARCANE_BARRAGE:
            {
                // Low mana cost spender - good efficiency
                if (charges >= 4)
                    return 0.8f; // Efficient at max charges
                else if (manaPercent < 30 && charges >= 2)
                    return 0.9f; // Mana conservation mode
                else
                    return 0.3f;
            }

            case ARCANE_BLAST:
            {
                // High mana cost builder - efficiency depends on mana and charges
                if (manaPercent > 50 && charges < 4)
                    return 0.6f; // Good efficiency when building with good mana
                else if (manaPercent < 30)
                    return 0.2f; // Poor efficiency at low mana
                else
                    return 0.4f;
            }

            case ARCANE_SURGE:
            {
                // Check if we have mana to sustain the surge window
                if (manaPercent >= 70)
                    return 0.9f; // Good mana for burst
                else if (manaPercent >= 50)
                    return 0.5f; // Risky but possible
                else
                    return 0.0f; // Not enough mana
            }

            default:
                return 0.5f; // Neutral resource efficiency
        }
    }

    /**
     * @brief Score strategic value of an action
     * Considers fight phase, cooldown saving, long-term optimization
     */
    float ScoreStrategic(uint32 actionId, uint32 charges, uint32 manaPercent) const
    {
        bot::ai::CombatContext context = _scoringEngine.GetContext();

        switch (actionId)
        {
            case ARCANE_SURGE:
            {
                // Save for important fights (bosses > trash)
                if (context == bot::ai::CombatContext::DUNGEON_BOSS ||
                    context == bot::ai::CombatContext::RAID_NORMAL ||
                    context == bot::ai::CombatContext::RAID_HEROIC)
                    return 1.0f; // Use on bosses
                else if (context == bot::ai::CombatContext::DUNGEON_TRASH)
                    return 0.3f; // Save for boss
                else if (context == bot::ai::CombatContext::GROUP)
                    return 0.6f; // Use on elite/rare mobs
                else
                    return 0.5f; // Solo: use freely
            }

            case TIME_WARP:
            {
                // Heroism/Bloodlust - save for critical moments
                if (context == bot::ai::CombatContext::RAID_HEROIC)
                    return 1.0f; // Critical raid boss
                else if (context == bot::ai::CombatContext::RAID_NORMAL)
                    return 0.8f;
                else if (context == bot::ai::CombatContext::DUNGEON_BOSS)
                    return 0.6f;
                else
                    return 0.0f; // Don't waste on trash/solo
            }

            case TOUCH_OF_MAGE:
            {
                // Damage amp debuff - good strategic value at 4 charges
                if (charges >= 4)
                    return 0.7f;
                else
                    return 0.3f;
            }

            case ARCANE_BARRAGE:
            {
                // Charge cycling - strategic for mana conservation
                if (charges >= 4 && manaPercent < 70)
                    return 0.6f; // Good cycle point
                else
                    return 0.3f;
            }

            default:
                return 0.5f; // Neutral strategic value
        }
    }

    /**
     * @brief Execute a scored action
     * Handles spell casting and state updates
     */
    void ExecuteAction(uint32 actionId, ::Unit* target)
    {
        Player* bot = this->GetBot();
        if (!bot)
            return;

        // Determine cast target
        ::Unit* castTarget = target;
        bool selfCast = false;

        // Self-cast abilities
        switch (actionId)
        {
            case ARCANE_SURGE:
            case PRESENCE_OF_MIND:
            case ICE_BLOCK:
            case MIRROR_IMAGE:
            case SHIFTING_POWER:
            case EVOCATION:
            case ARCANE_FAMILIAR:
            case ARCANE_INTELLECT:
            case TIME_WARP:
                castTarget = bot;
                selfCast = true;
                break;
            default:
                break;
        }

        // Attempt cast
        if (this->CanCastSpell(actionId, castTarget))
        {
            this->CastSpell(castTarget, actionId);

            // Update state based on action
            switch (actionId)
            {
                case ARCANE_BLAST:
                    _chargeTracker.AddCharge();
                    // Chance to proc Clearcasting
                    if (rand() % 100 < 10) // 10% chance (simplified)
                        _clearcastingTracker.ActivateProc();
                    break;

                case ARCANE_BARRAGE:
                    _chargeTracker.ClearCharges();
                    break;

                case ARCANE_MISSILES:
                    if (_clearcastingTracker.IsActive())
                        _clearcastingTracker.ConsumeProc();
                    break;

                case ARCANE_SURGE:
                    _arcaneSurgeActive = true;
                    _arcaneSurgeEndTime = GameTime::GetGameTimeMS() + 15000; // 15 sec
                    break;

                default:
                    break;
            }
        }
    }

    // Member variables
    bot::ai::ActionScoringEngine _scoringEngine;  ///< Utility-based action scoring engine
    uint32 _lastContextUpdate;                     ///< Last time context was updated
    uint32 _contextUpdateInterval;                 ///< Context update interval (milliseconds)
};

} // namespace Playerbot

#endif // PLAYERBOT_ARCANEMAGEWEIGHTED_H
