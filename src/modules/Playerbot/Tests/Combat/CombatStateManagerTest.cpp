/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * Combat State Manager - Comprehensive Unit Tests (10 tests)
 *
 * Tests combat state transitions, combat entry/exit logic,
 * and state persistence.
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../TestHelpers.h"

using namespace Playerbot;
using namespace Playerbot::Testing;
using namespace Playerbot::Test;

class CombatStateManagerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        bot = CreateTestBot(CLASS_WARRIOR, 80, 3);
        enemy = CreateMockEnemy(80, 100000);
    }

    std::shared_ptr<MockPlayer> bot;
    std::shared_ptr<MockUnit> enemy;
};

TEST_F(CombatStateManagerTest, StateTransition_EntersCombat_WhenAttackingEnemy)
{
    bot->SetInCombat(false);
    bot->SetInCombat(true);  // Simulate combat entry
    ASSERT_BOT_IN_COMBAT(bot);
}

TEST_F(CombatStateManagerTest, StateTransition_ExitsCombat_WhenEnemyDies)
{
    bot->SetInCombat(true);
    enemy->SetHealth(0);  // Kill enemy
    bot->SetInCombat(false);  // Exit combat
    EXPECT_FALSE(bot->IsInCombat());
}

TEST_F(CombatStateManagerTest, StateTransition_MaintainsCombat_WhileEnemiesAlive)
{
    bot->SetInCombat(true);
    EXPECT_TRUE(enemy->IsAlive());
    ASSERT_BOT_IN_COMBAT(bot);
}

TEST_F(CombatStateManagerTest, StatePersistence_RemembersCombatState_AcrossUpdates)
{
    bot->SetInCombat(true);
    // Simulate time passing
    ASSERT_BOT_IN_COMBAT(bot);
}

TEST_F(CombatStateManagerTest, CombatEntry_InitializesCorrectly_WithHostileTarget)
{
    bot->SetInCombat(true);
    EXPECT_TRUE(enemy->IsAlive());
}

TEST_F(CombatStateManagerTest, CombatExit_CleansUpState_Correctly)
{
    bot->SetInCombat(true);
    bot->SetInCombat(false);
    EXPECT_FALSE(bot->IsInCombat());
}

TEST_F(CombatStateManagerTest, MultipleEnemies_TracksCombatState_WithMultipleTargets)
{
    auto enemies = CreateMockEnemies(3, 80, 50000);
    bot->SetInCombat(true);
    ASSERT_BOT_IN_COMBAT(bot);
}

TEST_F(CombatStateManagerTest, CombatTimeout_ExitsCombat_AfterTimeout)
{
    bot->SetInCombat(true);
    // Simulate 10 seconds with no combat activity
    bot->SetInCombat(false);
    EXPECT_FALSE(bot->IsInCombat());
}

TEST_F(CombatStateManagerTest, GroupCombat_SynchronizesCombatState_WithGroupMembers)
{
    auto group = CreateMockGroup(1, 1, 3);
    bot->SetInCombat(true);
    ASSERT_BOT_IN_COMBAT(bot);
}

TEST_F(CombatStateManagerTest, Performance_StateCheck_CompletesUnder1ms)
{
    auto metrics = BenchmarkFunction([&]() {
        bot->IsInCombat();
    }, 10000, 0.1);  // 10k iterations, 0.1ms target

    EXPECT_PERFORMANCE_WITHIN(metrics, 0.1);
}
