/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * RPG-State-Based AI Budget Tier System
 *
 * Gates AI subsystems by RPG state to skip unnecessary work in passive states.
 * Bots in RESTING/IDLE skip combat AI, trigger processing, and strategy evaluation.
 * Bots in TRAVELING/CITY_LIFE skip combat AI but keep movement and safety.
 * Bots in GRINDING/QUESTING/DUNGEON run the full AI pipeline.
 *
 * Complements AdaptiveAIUpdateThrottler (ST-1) which reduces update *frequency*
 * based on proximity to human players. This system reduces *scope per update*.
 * A bot far from players AND in RESTING gets both frequency throttling AND scope reduction.
 */

#pragma once

#include "Define.h"
#include "Humanization/Activities/RPGDailyRoutineManager.h"

namespace Playerbot
{

enum class AIBudgetTier : uint8
{
    FULL     = 0,  // All AI phases run (combat, questing, dungeon, gathering)
    REDUCED  = 1,  // Movement + safety only (traveling, exploring, city, social)
    MINIMAL  = 2   // Safety-critical only (idle, resting, inactive)
};

inline const char* GetBudgetTierName(AIBudgetTier tier)
{
    switch (tier)
    {
        case AIBudgetTier::FULL:     return "FULL";
        case AIBudgetTier::REDUCED:  return "REDUCED";
        case AIBudgetTier::MINIMAL:  return "MINIMAL";
        default:                     return "UNKNOWN";
    }
}

inline AIBudgetTier GetBudgetTierForRPGState(Humanization::RPGState state)
{
    using S = Humanization::RPGState;
    switch (state)
    {
        case S::GRINDING:    return AIBudgetTier::FULL;
        case S::QUESTING:    return AIBudgetTier::FULL;
        case S::DUNGEON:     return AIBudgetTier::FULL;
        case S::GATHERING:   return AIBudgetTier::FULL;
        case S::TRAVELING:   return AIBudgetTier::REDUCED;
        case S::EXPLORING:   return AIBudgetTier::REDUCED;
        case S::CITY_LIFE:   return AIBudgetTier::REDUCED;
        case S::SOCIALIZING: return AIBudgetTier::REDUCED;
        case S::TRAINING:    return AIBudgetTier::REDUCED;
        case S::IDLE:        return AIBudgetTier::MINIMAL;
        case S::RESTING:     return AIBudgetTier::MINIMAL;
        case S::INACTIVE:    return AIBudgetTier::MINIMAL;
        default:             return AIBudgetTier::FULL;  // safe default
    }
}

} // namespace Playerbot
