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

namespace Playerbot
{

/**
 * SoloStrategy - Default strategy for solo bots
 *
 * Provides autonomous behavior for bots not in a group:
 * - Quest completion and progression
 * - Resource gathering (mining, herbalism, skinning)
 * - Autonomous combat with target acquisition
 * - Profession training and usage
 * - Trading and auction house activities
 * - World exploration
 *
 * This strategy is always active for solo bots and provides
 * a foundation for autonomous bot behavior. The name "Solo" reflects
 * that the bot is actively playing the game independently, not idle.
 */
class TC_GAME_API SoloStrategy : public Strategy
{
public:
    SoloStrategy();
    ~SoloStrategy() override = default;

    // Strategy interface
    void InitializeActions();
    void InitializeTriggers();
    void InitializeValues();

    // Activation
    void OnActivate(BotAI* ai);
    void OnDeactivate(BotAI* ai);

    // Always active for solo bots
    bool IsActive(BotAI* ai) const;

    // Update behavior
    void UpdateBehavior(BotAI* ai, uint32 diff);

private:
    uint32 _lastWanderTime = 0;
    uint32 _wanderInterval = 30000; // Wander every 30 seconds

    // Observer Pattern: Query manager states via atomic operations (<0.001ms each)
    // Managers self-throttle and update via BotAI::UpdateManagers()
    // SoloStrategy just observes their state changes through lock-free atomics
};

} // namespace Playerbot
