/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Strategy.h"
#include "Define.h"

class Item;

namespace Playerbot
{

// Forward declarations
class BotAI;

/**
 * @class RestStrategy
 * @brief Handles eating, drinking, and healing for solo bots
 *
 * This strategy drives bots to:
 * - Eat food when health is low
 * - Drink water when mana is low
 * - Use bandages when out of combat
 * - Rest until resources are full
 *
 * Priority: High (must rest before continuing activities)
 * Performance: <0.05ms per update (simple resource checks)
 */
class TC_GAME_API RestStrategy : public Strategy
{
public:
    RestStrategy();
    ~RestStrategy() override = default;

    // Strategy interface
    void InitializeActions();
    void InitializeTriggers();
    void InitializeValues();

    void OnActivate(BotAI* ai);
    void OnDeactivate(BotAI* ai);

    bool IsActive(BotAI* ai) const;
    float GetRelevance(BotAI* ai) const;
    void UpdateBehavior(BotAI* ai, uint32 diff);

private:
    /**
     * @brief Check if bot needs to eat
     * @param ai Bot AI instance
     * @return true if health below eating threshold
     */
    bool NeedsFood(BotAI* ai) const;

    /**
     * @brief Check if bot needs to drink
     * @param ai Bot AI instance
     * @return true if mana below drinking threshold
     */
    bool NeedsDrink(BotAI* ai) const;

    /**
     * @brief Find food in bot's inventory
     * @param ai Bot AI instance
     * @return Item pointer or nullptr
     */
    Item* FindFood(BotAI* ai) const;

    /**
     * @brief Find drink in bot's inventory
     * @param ai Bot AI instance
     * @return Item pointer or nullptr
     */
    Item* FindDrink(BotAI* ai) const;

    /**
     * @brief Find bandage in bot's inventory
     * @param ai Bot AI instance
     * @return Item pointer or nullptr
     */
    Item* FindBandage(BotAI* ai) const;

    /**
     * @brief Use food item
     * @param ai Bot AI instance
     * @param food Food item to use
     * @return true if successfully used
     */
    bool EatFood(BotAI* ai, Item* food);

    /**
     * @brief Use drink item
     * @param ai Bot AI instance
     * @param drink Drink item to use
     * @return true if successfully used
     */
    bool DrinkWater(BotAI* ai, Item* drink);

    /**
     * @brief Use bandage item
     * @param ai Bot AI instance
     * @param bandage Bandage item to use
     * @return true if successfully used
     */
    bool UseBandage(BotAI* ai, Item* bandage);

    /**
     * @brief Check if bot is currently resting (eating/drinking)
     * @param ai Bot AI instance
     * @return true if resting
     */
    bool IsResting(BotAI* ai) const;

private:
    // Resource thresholds
    float _eatHealthThreshold = 60.0f;   // Eat when below 60% health
    float _drinkManaThreshold = 40.0f;    // Drink when below 40% mana
    float _restCompleteHealth = 90.0f;    // Stop eating when above 90% health
    float _restCompleteMana = 90.0f;      // Stop drinking when above 90% mana

    // Resting state
    bool _isEating = false;
    bool _isDrinking = false;
    uint32 _restStartTime = 0;
    uint32 _maxRestTime = 30000; // Max 30 seconds of resting

    // Performance tracking
    uint32 _foodConsumed = 0;
    uint32 _drinkConsumed = 0;
    uint32 _bandagesUsed = 0;
};

} // namespace Playerbot
