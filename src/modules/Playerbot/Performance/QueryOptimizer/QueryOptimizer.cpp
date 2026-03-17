/*
 * Copyright (C) 2024 TrinityCore <http://www.trinitycore.org/>
 *
 * Phase 5: Performance Optimization - QueryOptimizer Implementation
 */

#include "QueryOptimizer.h"
#include "Log.h"

namespace Playerbot {
namespace Performance {

QueryOptimizer::QueryOptimizer(Configuration config)
    : _config(config)
{
    TC_LOG_INFO("playerbot.performance", "QueryOptimizer initialized");
}

QueryOptimizer& QueryOptimizer::Instance()
{
    static QueryOptimizer instance(Configuration{});
    return instance;
}

} // namespace Performance
} // namespace Playerbot
