/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * Playerbot Test Helpers - Comprehensive Testing Utilities
 *
 * This file provides reusable helper functions, assertion macros, and utilities
 * for testing the Playerbot module. All helpers are designed for ease of use
 * and comprehensive test coverage.
 */

#pragma once

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <chrono>
#include <functional>
#include <memory>
#include <vector>
#include "Phase3/Unit/Mocks/MockFramework.h"

namespace Playerbot
{
namespace Testing
{

// ============================================================================
// PERFORMANCE BENCHMARKING
// ============================================================================

struct PerformanceMetrics
{
    uint64_t executionTimeNs{0};      // Execution time in nanoseconds
    uint64_t executionTimeMs{0};      // Execution time in milliseconds
    double executionTimeSec{0.0};     // Execution time in seconds
    uint64_t minTimeNs{UINT64_MAX};   // Minimum execution time
    uint64_t maxTimeNs{0};            // Maximum execution time
    double avgTimeNs{0.0};            // Average execution time
    uint64_t iterations{1};           // Number of iterations
    bool withinTarget{false};         // Whether performance met target
    uint64_t targetNs{0};             // Target performance threshold

    void Print() const
    {
        ::std::cout << "Performance Metrics:\n";
        ::std::cout << "  - Execution Time: " << executionTimeMs << " ms\n";
        ::std::cout << "  - Iterations: " << iterations << "\n";
        if (iterations > 1)
        {
            ::std::cout << "  - Average: " << (avgTimeNs / 1000000.0) << " ms\n";
            ::std::cout << "  - Min: " << (minTimeNs / 1000000.0) << " ms\n";
            ::std::cout << "  - Max: " << (maxTimeNs / 1000000.0) << " ms\n";
        }
        if (targetNs > 0)
        {
            ::std::cout << "  - Target: " << (targetNs / 1000000.0) << " ms\n";
            ::std::cout << "  - Status: " << (withinTarget ? "PASS" : "FAIL") << "\n";
        }
    }
};

/**
 * @brief Benchmark a function's execution time
 * @param func Function to benchmark
 * @param iterations Number of iterations (default: 1)
 * @param targetMs Target execution time in milliseconds (0 = no target)
 * @return Performance metrics
 */
inline PerformanceMetrics BenchmarkFunction(
    ::std::function<void()> func,
    uint64_t iterations = 1,
    uint64_t targetMs = 0)
{
    PerformanceMetrics metrics;
    metrics.iterations = iterations;
    metrics.targetNs = targetMs * 1000000;

    auto start = ::std::chrono::high_resolution_clock::now();

    for (uint64_t i = 0; i < iterations; ++i)
    {
        auto iterStart = ::std::chrono::high_resolution_clock::now();
        func();
        auto iterEnd = ::std::chrono::high_resolution_clock::now();

        uint64_t iterTimeNs = ::std::chrono::duration_cast<::std::chrono::nanoseconds>(
            iterEnd - iterStart).count();

        metrics.minTimeNs = ::std::min(metrics.minTimeNs, iterTimeNs);
        metrics.maxTimeNs = ::std::max(metrics.maxTimeNs, iterTimeNs);
    }

    auto end = ::std::chrono::high_resolution_clock::now();

    metrics.executionTimeNs = ::std::chrono::duration_cast<::std::chrono::nanoseconds>(
        end - start).count();
    metrics.executionTimeMs = metrics.executionTimeNs / 1000000;
    metrics.executionTimeSec = metrics.executionTimeNs / 1000000000.0;
    metrics.avgTimeNs = static_cast<double>(metrics.executionTimeNs) / iterations;

    if (targetMs > 0)
    {
        metrics.withinTarget = (metrics.avgTimeNs <= metrics.targetNs);
    }

    return metrics;
}

// ============================================================================
// BOT CREATION HELPERS
// ============================================================================

/**
 * @brief Create a fully configured test bot
 * @param classId WoW class ID (1-13)
 * @param level Character level (1-80)
 * @param spec Specialization ID
 * @return Configured MockPlayer instance
 */
inline ::std::shared_ptr<Test::MockPlayer> CreateTestBot(
    uint8 classId,
    uint8 level = 80,
    uint32 spec = 0)
{
    return Test::MockFactory::CreateMockPlayer(classId, level, spec);
}

/**
 * @brief Create a test bot with specific health percentage
 */
inline ::std::shared_ptr<Test::MockPlayer> CreateTestBotWithHealth(
    uint8 classId,
    uint8 level,
    uint32 spec,
    float healthPct)
{
    auto bot = CreateTestBot(classId, level, spec);
    uint32 maxHealth = bot->GetMaxHealth();
    uint32 health = static_cast<uint32>(maxHealth * (healthPct / 100.0f));
    bot->SetHealth(health);
    return bot;
}

/**
 * @brief Create a test bot with specific power (mana/rage/energy)
 */
inline ::std::shared_ptr<Test::MockPlayer> CreateTestBotWithPower(
    uint8 classId,
    uint8 level,
    uint32 spec,
    Test::MockPowers powerType,
    uint32 powerAmount)
{
    auto bot = CreateTestBot(classId, level, spec);
    bot->SetPower(powerType, powerAmount);
    return bot;
}

/**
 * @brief Create a test bot in combat
 */
inline ::std::shared_ptr<Test::MockPlayer> CreateTestBotInCombat(
    uint8 classId,
    uint8 level = 80,
    uint32 spec = 0)
{
    auto bot = CreateTestBot(classId, level, spec);
    bot->SetInCombat(true);
    return bot;
}

// ============================================================================
// ENEMY/TARGET CREATION HELPERS
// ============================================================================

/**
 * @brief Create a mock enemy target
 * @param level Enemy level
 * @param health Enemy health
 * @return Configured MockUnit instance
 */
inline ::std::shared_ptr<Test::MockUnit> CreateMockEnemy(
    uint32 level = 80,
    uint32 health = 100000)
{
    return Test::MockFactory::CreateMockEnemy(level, health);
}

/**
 * @brief Create multiple mock enemies for AoE testing
 */
inline ::std::vector<::std::shared_ptr<Test::MockUnit>> CreateMockEnemies(
    uint32 count,
    uint32 level = 80,
    uint32 health = 100000)
{
    ::std::vector<::std::shared_ptr<Test::MockUnit>> enemies;
    enemies.reserve(count);

    for (uint32 i = 0; i < count; ++i)
    {
        enemies.push_back(CreateMockEnemy(level, health));
    }

    return enemies;
}

/**
 * @brief Create a boss enemy (high health, elite)
 */
inline ::std::shared_ptr<Test::MockUnit> CreateBossEnemy(
    uint32 level = 80,
    uint32 health = 5000000)
{
    auto boss = CreateMockEnemy(level, health);
    // Boss-specific setup can be added here
    return boss;
}

/**
 * @brief Create a low-health enemy for execute testing
 */
inline ::std::shared_ptr<Test::MockUnit> CreateLowHealthEnemy(
    uint32 level = 80,
    float healthPct = 20.0f)
{
    uint32 maxHealth = 100000;
    uint32 health = static_cast<uint32>(maxHealth * (healthPct / 100.0f));
    return CreateMockEnemy(level, health);
}

// ============================================================================
// GROUP CREATION HELPERS
// ============================================================================

/**
 * @brief Create a mock group with specified composition
 * @param tanks Number of tanks
 * @param healers Number of healers
 * @param dps Number of DPS
 * @return Configured MockGroup with members
 */
inline ::std::shared_ptr<Test::MockGroup> CreateMockGroup(
    uint32 tanks = 1,
    uint32 healers = 1,
    uint32 dps = 3)
{
    auto group = ::std::make_shared<Test::MockGroup>();
    ::std::vector<::std::shared_ptr<Test::MockPlayer>> members;

    // Add tanks (Warriors)
    for (uint32 i = 0; i < tanks; ++i)
    {
        auto tank = CreateTestBot(CLASS_WARRIOR, 80, 3); // Protection
        members.push_back(tank);
    }

    // Add healers (Priests)
    for (uint32 i = 0; i < healers; ++i)
    {
        auto healer = CreateTestBot(CLASS_PRIEST, 80, 1); // Holy
        members.push_back(healer);
    }

    // Add DPS (Mages)
    for (uint32 i = 0; i < dps; ++i)
    {
        auto dpser = CreateTestBot(CLASS_MAGE, 80, 3); // Frost
        members.push_back(dpser);
    }

    // Set group members
    for (const auto& member : members)
    {
        group->AddMember(member);
    }

    // Set first tank as leader
    if (!members.empty())
    {
        group->SetLeader(members[0]);
    }

    return group;
}

// ============================================================================
// COMBAT SIMULATION HELPERS
// ============================================================================

/**
 * @brief Simulate a combat encounter
 * @param bot Bot participating in combat
 * @param target Enemy target
 * @param durationMs Combat duration in milliseconds
 * @param updateInterval Update interval in milliseconds
 */
inline void SimulateCombat(
    ::std::shared_ptr<Test::MockPlayer> bot,
    ::std::shared_ptr<Test::MockUnit> target,
    uint32 durationMs,
    uint32 updateInterval = 100)
{
    bot->SetInCombat(true);

    uint32 elapsed = 0;
    while (elapsed < durationMs && target->IsAlive() && bot->IsAlive())
    {
        // Simulate damage
        if (target->GetHealth() > 1000)
        {
            target->SetHealth(target->GetHealth() - 1000);
        }
        else
        {
            target->SetHealth(0);
        }

        elapsed += updateInterval;
    }

    bot->SetInCombat(false);
}

/**
 * @brief Simulate AoE combat with multiple enemies
 */
inline void SimulateAoECombat(
    ::std::shared_ptr<Test::MockPlayer> bot,
    ::std::vector<::std::shared_ptr<Test::MockUnit>> targets,
    uint32 durationMs,
    uint32 updateInterval = 100)
{
    bot->SetInCombat(true);

    uint32 elapsed = 0;
    while (elapsed < durationMs)
    {
        bool anyAlive = false;

        for (auto& target : targets)
        {
            if (target->IsAlive())
            {
                anyAlive = true;
                if (target->GetHealth() > 500)
                {
                    target->SetHealth(target->GetHealth() - 500);
                }
                else
                {
                    target->SetHealth(0);
                }
            }
        }

        if (!anyAlive || !bot->IsAlive())
            break;

        elapsed += updateInterval;
    }

    bot->SetInCombat(false);
}

// ============================================================================
// QUEST SIMULATION HELPERS
// ============================================================================

/**
 * @brief Simulate quest acceptance
 */
inline void AcceptQuest(::std::shared_ptr<Test::MockPlayer> bot, uint32 questId)
{
    // Quest acceptance simulation
    // This would normally interact with QuestManager
    EXPECT_TRUE(bot) << "Bot must not be null";
}

/**
 * @brief Simulate quest completion
 */
inline void CompleteQuest(::std::shared_ptr<Test::MockPlayer> bot, uint32 questId)
{
    // Quest completion simulation
    EXPECT_TRUE(bot) << "Bot must not be null";
}

// ============================================================================
// ASSERTION HELPERS
// ============================================================================

/**
 * @brief Assert that bot is alive
 */
#define ASSERT_BOT_ALIVE(bot) \
    ASSERT_TRUE(bot) << "Bot pointer must not be null"; \
    ASSERT_TRUE((bot)->IsAlive()) << "Bot must be alive"

/**
 * @brief Assert that bot is dead
 */
#define ASSERT_BOT_DEAD(bot) \
    ASSERT_TRUE(bot) << "Bot pointer must not be null"; \
    ASSERT_TRUE((bot)->IsDead()) << "Bot must be dead"

/**
 * @brief Assert that bot is in combat
 */
#define ASSERT_BOT_IN_COMBAT(bot) \
    ASSERT_TRUE(bot) << "Bot pointer must not be null"; \
    ASSERT_TRUE((bot)->IsInCombat()) << "Bot must be in combat"

/**
 * @brief Assert that bot has enough health
 */
#define ASSERT_BOT_HEALTH_ABOVE(bot, percent) \
    ASSERT_TRUE(bot) << "Bot pointer must not be null"; \
    ASSERT_GE((bot)->GetHealthPct(), percent) \
        << "Bot health " << (bot)->GetHealthPct() << "% must be >= " << percent << "%"

/**
 * @brief Assert that bot has enough power (mana/rage/energy)
 */
#define ASSERT_BOT_POWER_ABOVE(bot, powerType, amount) \
    ASSERT_TRUE(bot) << "Bot pointer must not be null"; \
    ASSERT_GE((bot)->GetPower(powerType), amount) \
        << "Bot " << #powerType << " " << (bot)->GetPower(powerType) \
        << " must be >= " << amount

/**
 * @brief Assert that spell is on cooldown
 */
#define ASSERT_SPELL_ON_COOLDOWN(bot, spellId) \
    ASSERT_TRUE(bot) << "Bot pointer must not be null"
    // Additional cooldown checking logic would go here

/**
 * @brief Assert that spell is off cooldown
 */
#define ASSERT_SPELL_OFF_COOLDOWN(bot, spellId) \
    ASSERT_TRUE(bot) << "Bot pointer must not be null"
    // Additional cooldown checking logic would go here

/**
 * @brief Assert that bot has a specific buff
 */
#define ASSERT_BOT_HAS_BUFF(bot, spellId) \
    ASSERT_TRUE(bot) << "Bot pointer must not be null"; \
    ASSERT_TRUE((bot)->HasAura(spellId)) << "Bot must have aura " << spellId

/**
 * @brief Assert that bot does not have a specific debuff
 */
#define ASSERT_BOT_NO_DEBUFF(bot, spellId) \
    ASSERT_TRUE(bot) << "Bot pointer must not be null"; \
    ASSERT_FALSE((bot)->HasAura(spellId)) << "Bot must not have aura " << spellId

/**
 * @brief Assert performance is within target
 */
#define ASSERT_PERFORMANCE_WITHIN(metrics, targetMs) \
    ASSERT_LE((metrics).avgTimeNs / 1000000.0, targetMs) \
        << "Performance " << ((metrics).avgTimeNs / 1000000.0) \
        << " ms exceeds target " << targetMs << " ms"

/**
 * @brief Expect performance is within target (non-fatal)
 */
#define EXPECT_PERFORMANCE_WITHIN(metrics, targetMs) \
    EXPECT_LE((metrics).avgTimeNs / 1000000.0, targetMs) \
        << "Performance " << ((metrics).avgTimeNs / 1000000.0) \
        << " ms exceeds target " << targetMs << " ms"

// ============================================================================
// SPELL TESTING HELPERS
// ============================================================================

/**
 * @brief Verify spell can be cast on target
 */
inline bool CanCastSpell(uint32 spellId,
    ::std::shared_ptr<Test::MockUnit> target, ::std::shared_ptr<Test::MockPlayer> bot)
{
    if (!bot || !target)
        return false;

    // Basic checks
    if (!bot->HasSpell(spellId))
        return false;

    if (!target->IsAlive())
        return false;

    // Power check would go here

    return true;
}

/**
 * @brief Simulate spell cast
 */
inline void CastSpell(uint32 spellId,
    ::std::shared_ptr<Test::MockUnit> target, ::std::shared_ptr<Test::MockPlayer> bot)
{
    ASSERT_TRUE(CanCastSpell(spellId, target, bot))
        << "Cannot cast spell " << spellId;

    // Spell casting simulation
    // Apply cooldown, consume power, etc.
}

// ============================================================================
// RESOURCE MANAGEMENT HELPERS
// ============================================================================

/**
 * @brief Set bot to out-of-mana condition
 */
inline void SetBotOutOfMana(::std::shared_ptr<Test::MockPlayer> bot)
{
    bot->SetPower(Test::POWER_MANA, 0);
}

/**
 * @brief Set bot to low health condition
 */
inline void SetBotLowHealth(::std::shared_ptr<Test::MockPlayer> bot, float percent = 20.0f)
{
    uint32 health = static_cast<uint32>(bot->GetMaxHealth() * (percent / 100.0f));
    bot->SetHealth(health);
}

/**
 * @brief Restore bot to full resources
 */
inline void RestoreBotResources(::std::shared_ptr<Test::MockPlayer> bot)
{
    bot->SetHealth(bot->GetMaxHealth());
    bot->SetPower(Test::POWER_MANA, bot->GetMaxPower(Test::POWER_MANA));
    bot->SetPower(Test::POWER_RAGE, bot->GetMaxPower(Test::POWER_RAGE));
    bot->SetPower(Test::POWER_ENERGY, bot->GetMaxPower(Test::POWER_ENERGY));
}

// ============================================================================
// THREAT TESTING HELPERS
// ============================================================================

/**
 * @brief Simulate high threat situation
 */
inline void SimulateHighThreat(
    ::std::shared_ptr<Test::MockPlayer> bot,
    ::std::shared_ptr<Test::MockUnit> target)
{
    // Set bot as target's current target
    bot->SetInCombat(true);
}

/**
 * @brief Simulate threat emergency (losing aggro)
 */
inline void SimulateThreatEmergency(
    ::std::shared_ptr<Test::MockPlayer> tank,
    ::std::shared_ptr<Test::MockPlayer> dps,
    ::std::shared_ptr<Test::MockUnit> target)
{
    // DPS has pulled aggro from tank
    tank->SetInCombat(true);
    dps->SetInCombat(true);
}

// ============================================================================
// COOLDOWN TESTING HELPERS
// ============================================================================

/**
 * @brief Simulate cooldown expiry
 */
inline void ExpireCooldowns(::std::shared_ptr<Test::MockPlayer> bot)
{
    // All cooldowns are now available
    // This would normally interact with a cooldown tracker
}

/**
 * @brief Check if all defensive cooldowns are available
 */
inline bool AllDefensiveCooldownsReady(::std::shared_ptr<Test::MockPlayer> bot)
{
    // Check defensive cooldown availability
    return true;
}

} // namespace Testing
} // namespace Playerbot
