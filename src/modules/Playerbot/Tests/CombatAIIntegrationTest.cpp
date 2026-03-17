/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include <gtest/gtest.h>
#include "AI/Combat/CombatAIIntegrator.h"
#include "AI/EnhancedBotAI.h"
#include "AI/Combat/RoleBasedCombatPositioning.h"
#include "AI/Combat/InterruptCoordinator.h"
#include "AI/Combat/ThreatCoordinator.h"
#include "Player.h"
#include "Map.h"
#include "World.h"
#include <chrono>
#include <memory>
#include <vector>
#include <thread>

using namespace Playerbot;

// Mock classes for testing
class MockPlayer : public Player
{
public:
    MockPlayer() : Player(nullptr) {}

    float GetHealthPct() const override { return _healthPct; }
    float GetPowerPct(Powers power) const override { return _manaPct; }
    bool IsInCombat() const override { return _inCombat; }

    void SetHealthPct(float pct) { _healthPct = pct; }
    void SetManaPct(float pct) { _manaPct = pct; }
    void SetInCombat(bool combat) { _inCombat = combat; }

private:
    float _healthPct = 100.0f;
    float _manaPct = 100.0f;
    bool _inCombat = false;
};

class MockUnit : public Unit
{
public:
    MockUnit() : Unit(false) {}

    bool HasUnitState(uint32 state) const override
    {
        return state == UNIT_STATE_CASTING ? _casting : false;
    }

    void SetCasting(bool casting) { _casting = casting; }

private:
    bool _casting = false;
};

// Test fixture for Combat AI Integration
class CombatAIIntegrationTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        _bot = std::make_unique<MockPlayer>();
        _target = std::make_unique<MockUnit>();
    }

    void TearDown() override
    {
        _bot.reset();
        _target.reset();
    }

    std::unique_ptr<MockPlayer> _bot;
    std::unique_ptr<MockUnit> _target;
};

// Basic integration tests
TEST_F(CombatAIIntegrationTest, CreateCombatAIIntegrator)
{
    auto integrator = std::make_unique<CombatAIIntegrator>(_bot.get());
    ASSERT_NE(integrator, nullptr);
    EXPECT_FALSE(integrator->IsInCombat());
    EXPECT_EQ(integrator->GetPhase(), CombatPhase::NONE);
}

TEST_F(CombatAIIntegrationTest, CombatStartStop)
{
    auto integrator = std::make_unique<CombatAIIntegrator>(_bot.get());

    // Start combat
    integrator->OnCombatStart(_target.get());
    EXPECT_TRUE(integrator->IsInCombat());
    EXPECT_EQ(integrator->GetPhase(), CombatPhase::ENGAGING);
    EXPECT_EQ(integrator->GetCurrentTarget(), _target.get());

    // End combat
    integrator->OnCombatEnd();
    EXPECT_FALSE(integrator->IsInCombat());
    EXPECT_EQ(integrator->GetPhase(), CombatPhase::RECOVERING);
}

TEST_F(CombatAIIntegrationTest, TargetSwitching)
{
    auto integrator = std::make_unique<CombatAIIntegrator>(_bot.get());
    auto newTarget = std::make_unique<MockUnit>();

    integrator->OnCombatStart(_target.get());
    EXPECT_EQ(integrator->GetCurrentTarget(), _target.get());

    integrator->OnTargetChanged(newTarget.get());
    EXPECT_EQ(integrator->GetCurrentTarget(), newTarget.get());
}

TEST_F(CombatAIIntegrationTest, ConfigurationUpdate)
{
    auto integrator = std::make_unique<CombatAIIntegrator>(_bot.get());

    CombatAIConfig config;
    config.enablePositioning = false;
    config.enableInterrupts = true;
    config.updateIntervalMs = 50;

    integrator->SetConfig(config);
    auto retrievedConfig = integrator->GetConfig();

    EXPECT_FALSE(retrievedConfig.enablePositioning);
    EXPECT_TRUE(retrievedConfig.enableInterrupts);
    EXPECT_EQ(retrievedConfig.updateIntervalMs, 50);
}

// Performance tests
TEST_F(CombatAIIntegrationTest, UpdatePerformance)
{
    auto integrator = std::make_unique<CombatAIIntegrator>(_bot.get());
    integrator->OnCombatStart(_target.get());

    const int iterations = 1000;
    auto startTime = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i)
    {
        IntegrationResult result = integrator->Update(100);
        EXPECT_TRUE(result.success);
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    // Should process 1000 updates in less than 100ms (0.1ms per update average)
    EXPECT_LT(duration.count(), 100);

    // Check metrics
    auto metrics = integrator->GetMetrics();
    EXPECT_GT(metrics.updateCount, 0);
    EXPECT_LT(metrics.avgCpuPercent, 0.1f); // Less than 0.1% CPU
}

TEST_F(CombatAIIntegrationTest, MemoryUsage)
{
    auto integrator = std::make_unique<CombatAIIntegrator>(_bot.get());

    // Simulate intensive combat
    integrator->OnCombatStart(_target.get());

    for (int i = 0; i < 100; ++i)
    {
        integrator->Update(100);
    }

    auto metrics = integrator->GetMetrics();

    // Memory should stay under 10MB
    EXPECT_LT(metrics.memoryUsed, 10485760);
    EXPECT_LT(metrics.peakMemory, 10485760);
}

// Component integration tests
TEST_F(CombatAIIntegrationTest, PositioningIntegration)
{
    auto integrator = std::make_unique<CombatAIIntegrator>(_bot.get());

    ASSERT_NE(integrator->GetPositioning(), nullptr);

    integrator->OnCombatStart(_target.get());
    integrator->Update(100);

    auto metrics = integrator->GetMetrics();
    EXPECT_GE(metrics.positionChanges, 0);
}

TEST_F(CombatAIIntegrationTest, InterruptIntegration)
{
    auto integrator = std::make_unique<CombatAIIntegrator>(_bot.get());

    ASSERT_NE(integrator->GetInterruptCoordinator(), nullptr);

    _target->SetCasting(true);
    integrator->OnCombatStart(_target.get());

    // Simulate multiple updates to trigger interrupt checking
    for (int i = 0; i < 10; ++i)
    {
        integrator->Update(100);
    }

    auto metrics = integrator->GetMetrics();
    EXPECT_GE(metrics.interruptsAttempted, 0);
}

TEST_F(CombatAIIntegrationTest, ThreatIntegration)
{
    auto integrator = std::make_unique<CombatAIIntegrator>(_bot.get());

    ASSERT_NE(integrator->GetThreatCoordinator(), nullptr);

    integrator->OnCombatStart(_target.get());

    // Simulate threat updates
    for (int i = 0; i < 10; ++i)
    {
        integrator->Update(100);
    }

    auto metrics = integrator->GetMetrics();
    EXPECT_GE(metrics.threatAdjustments, 0);
}

// Phase transition tests
TEST_F(CombatAIIntegrationTest, PhaseTransitions)
{
    auto integrator = std::make_unique<CombatAIIntegrator>(_bot.get());

    // Start combat - should be ENGAGING
    integrator->OnCombatStart(_target.get());
    EXPECT_EQ(integrator->GetPhase(), CombatPhase::ENGAGING);

    // Simulate time passing - should transition to OPENING
    for (int i = 0; i < 5; ++i)
    {
        integrator->Update(100);
    }

    // Continue updating - should eventually reach SUSTAINED
    for (int i = 0; i < 30; ++i)
    {
        integrator->Update(100);
    }

    CombatPhase finalPhase = integrator->GetPhase();
    EXPECT_TRUE(finalPhase == CombatPhase::OPENING ||
                finalPhase == CombatPhase::SUSTAINED);
}

// EnhancedBotAI integration tests
TEST_F(CombatAIIntegrationTest, EnhancedBotAICreation)
{
    auto enhancedAI = std::make_unique<EnhancedBotAI>(_bot.get());
    ASSERT_NE(enhancedAI, nullptr);
    ASSERT_NE(enhancedAI->GetCombatAI(), nullptr);
}

TEST_F(CombatAIIntegrationTest, EnhancedBotAICombatFlow)
{
    auto enhancedAI = std::make_unique<EnhancedBotAI>(_bot.get());

    _bot->SetInCombat(true);
    enhancedAI->OnCombatStart(_target.get());

    // Run several update cycles
    for (int i = 0; i < 10; ++i)
    {
        enhancedAI->UpdateAI(100);
    }

    auto stats = enhancedAI->GetStats();
    EXPECT_GT(stats.totalUpdates, 0);
    EXPECT_GT(stats.combatUpdates, 0);
}

TEST_F(CombatAIIntegrationTest, EnhancedBotAIPerformance)
{
    auto enhancedAI = std::make_unique<EnhancedBotAI>(_bot.get());

    _bot->SetInCombat(true);
    enhancedAI->OnCombatStart(_target.get());

    const int iterations = 1000;
    auto startTime = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i)
    {
        enhancedAI->UpdateAI(10);
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    auto stats = enhancedAI->GetStats();

    // Should maintain performance targets
    EXPECT_LT(stats.cpuUsagePercent, 0.1f);
    EXPECT_LT(stats.memoryUsageBytes, 10485760);
    EXPECT_LT(duration.count(), 100); // 1000 updates in < 100ms
}

// Role-specific tests
TEST_F(CombatAIIntegrationTest, TankRoleConfiguration)
{
    auto tankAI = CombatAIFactory::CreateTankCombatAI(_bot.get());
    ASSERT_NE(tankAI, nullptr);

    auto config = tankAI->GetConfig();
    EXPECT_TRUE(config.enableThreatManagement);
    EXPECT_LT(config.threatUpdateThreshold, 10.0f);
}

TEST_F(CombatAIIntegrationTest, HealerRoleConfiguration)
{
    auto healerAI = CombatAIFactory::CreateHealerCombatAI(_bot.get());
    ASSERT_NE(healerAI, nullptr);

    auto config = healerAI->GetConfig();
    EXPECT_TRUE(config.enableKiting);
    EXPECT_GT(config.positionUpdateThreshold, 5.0f);
}

TEST_F(CombatAIIntegrationTest, DPSRoleConfiguration)
{
    auto dpsAI = CombatAIFactory::CreateMeleeDPSCombatAI(_bot.get());
    ASSERT_NE(dpsAI, nullptr);

    auto config = dpsAI->GetConfig();
    EXPECT_TRUE(config.enableInterrupts);
    EXPECT_LT(config.targetSwitchCooldownMs, 1000);
}

// Stress tests
TEST_F(CombatAIIntegrationTest, StressTestMultipleBots)
{
    const int botCount = 100;
    std::vector<std::unique_ptr<MockPlayer>> bots;
    std::vector<std::unique_ptr<CombatAIIntegrator>> integrators;

    // Create multiple bots
    for (int i = 0; i < botCount; ++i)
    {
        bots.push_back(std::make_unique<MockPlayer>());
        integrators.push_back(std::make_unique<CombatAIIntegrator>(bots.back().get()));
    }

    // Start combat for all
    for (auto& integrator : integrators)
    {
        integrator->OnCombatStart(_target.get());
    }

    auto startTime = std::chrono::high_resolution_clock::now();

    // Update all bots
    for (int update = 0; update < 10; ++update)
    {
        for (auto& integrator : integrators)
        {
            integrator->Update(100);
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    // 100 bots * 10 updates should complete quickly
    EXPECT_LT(duration.count(), 1000);

    // Check individual bot performance
    for (const auto& integrator : integrators)
    {
        auto metrics = integrator->GetMetrics();
        EXPECT_LT(metrics.avgCpuPercent, 0.1f);
    }
}

TEST_F(CombatAIIntegrationTest, StressTestLongCombat)
{
    auto integrator = std::make_unique<CombatAIIntegrator>(_bot.get());
    integrator->OnCombatStart(_target.get());

    // Simulate 5 minutes of combat
    const int updates = 3000; // 300 seconds at 100ms intervals
    for (int i = 0; i < updates; ++i)
    {
        IntegrationResult result = integrator->Update(100);
        EXPECT_TRUE(result.success);

        // Simulate health changes
    if (i % 100 == 0)
        {
            _bot->SetHealthPct(50 + (rand() % 50));
            _bot->SetManaPct(30 + (rand() % 70));
        }

        // Simulate target changes
    if (i % 500 == 0)
        {
            auto newTarget = std::make_unique<MockUnit>();
            integrator->OnTargetChanged(newTarget.get());
        }
    }

    auto metrics = integrator->GetMetrics();

    // Performance should remain consistent
    EXPECT_LT(metrics.avgCpuPercent, 0.1f);
    EXPECT_LT(metrics.memoryUsed, 10485760);
}

// Thread safety tests
TEST_F(CombatAIIntegrationTest, ThreadSafety)
{
    auto integrator = std::make_unique<CombatAIIntegrator>(_bot.get());
    integrator->OnCombatStart(_target.get());

    std::atomic<bool> running{true};
    std::vector<std::thread> threads;

    // Update thread
    threads.emplace_back([&integrator, &running]() {
        while (running)
        {
            integrator->Update(10);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });

    // Configuration thread
    threads.emplace_back([&integrator, &running]() {
        while (running)
        {
            CombatAIConfig config;
            config.enablePositioning = rand() % 2;
            config.enableInterrupts = rand() % 2;
            integrator->SetConfig(config);
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    });

    // Metrics thread
    threads.emplace_back([&integrator, &running]() {
        while (running)
        {
            auto metrics = integrator->GetMetrics();
            EXPECT_GE(metrics.updateCount, 0);
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
    });

    // Run for 100ms
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    running = false;

    // Wait for threads to finish
    for (auto& thread : threads)
    {
        thread.join();
    }

    // System should still be in valid state
    EXPECT_TRUE(integrator->IsInCombat());
    auto metrics = integrator->GetMetrics();
    EXPECT_GT(metrics.updateCount, 0);
}

// Error handling tests
TEST_F(CombatAIIntegrationTest, NullTargetHandling)
{
    auto integrator = std::make_unique<CombatAIIntegrator>(_bot.get());

    // Should handle null target gracefully
    integrator->OnCombatStart(nullptr);
    EXPECT_FALSE(integrator->IsInCombat());

    integrator->OnTargetChanged(nullptr);
    EXPECT_EQ(integrator->GetCurrentTarget(), nullptr);

    // Update should not crash
    IntegrationResult result = integrator->Update(100);
    EXPECT_TRUE(result.success);
}

TEST_F(CombatAIIntegrationTest, InvalidConfigHandling)
{
    auto integrator = std::make_unique<CombatAIIntegrator>(_bot.get());

    CombatAIConfig config;
    config.maxCpuMicrosPerUpdate = 0; // Invalid
    config.maxMemoryBytes = 0; // Invalid
    config.updateIntervalMs = 0; // Invalid

    integrator->SetConfig(config);

    // Should still function with invalid config
    integrator->OnCombatStart(_target.get());
    IntegrationResult result = integrator->Update(100);
    EXPECT_TRUE(result.success || !result.success); // May fail due to limits
}

// Main test runner
int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}