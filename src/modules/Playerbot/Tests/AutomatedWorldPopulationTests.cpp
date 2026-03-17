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
 * Integration Tests for Automated World Population System
 *
 * Tests all 4 subsystems and the BotLevelManager orchestrator:
 * - BotLevelDistribution
 * - BotGearFactory
 * - BotTalentManager
 * - BotWorldPositioner
 * - BotLevelManager (orchestrator)
 *
 * NOTE: Requires Google Test framework (not yet configured)
 * These tests are ready to run once Google Test is integrated
 */

#include "BotLevelDistribution.h"
#include "BotGearFactory.h"
#include "BotTalentManager.h"
#include "BotWorldPositioner.h"
#include "BotLevelManager.h"
#include "Player.h"
#include "World.h"
#include <gtest/gtest.h>
#include <vector>
#include <chrono>

using namespace Playerbot;

// ====================================================================
// TEST FIXTURE - Automated World Population System
// ====================================================================

class AutomatedWorldPopulationTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Initialize all subsystems
        ASSERT_TRUE(sBotLevelDistribution->LoadDistribution()) << "Failed to load level distribution";
        ASSERT_TRUE(sBotGearFactory->LoadGear()) << "Failed to load gear factory";
        ASSERT_TRUE(sBotTalentManager->LoadLoadouts()) << "Failed to load talent manager";
        ASSERT_TRUE(sBotWorldPositioner->LoadZones()) << "Failed to load world positioner";
        ASSERT_TRUE(sBotLevelManager->Initialize()) << "Failed to initialize level manager";
    }

    void TearDown() override
    {
        sBotLevelManager->Shutdown();
    }
};

// ====================================================================
// LEVEL DISTRIBUTION TESTS
// ====================================================================

TEST_F(AutomatedWorldPopulationTest, LevelDistribution_LoadsSuccessfully)
{
    EXPECT_TRUE(sBotLevelDistribution->IsReady());

    auto stats = sBotLevelDistribution->GetStats();
    EXPECT_GT(stats.totalBrackets, 0u) << "No brackets loaded";
    EXPECT_EQ(stats.allianceBrackets, 17u) << "Should have 17 Alliance brackets";
    EXPECT_EQ(stats.hordeBrackets, 17u) << "Should have 17 Horde brackets";
}

TEST_F(AutomatedWorldPopulationTest, LevelDistribution_SelectsValidBracket)
{
    // Test Alliance bracket selection
    auto allianceBracket = sBotLevelDistribution->SelectBracketWeighted(TEAM_ALLIANCE);
    ASSERT_NE(allianceBracket, nullptr) << "Failed to select Alliance bracket";
    EXPECT_GE(allianceBracket->minLevel, 1u);
    EXPECT_LE(allianceBracket->maxLevel, 80u);
    EXPECT_EQ(allianceBracket->faction, TEAM_ALLIANCE);

    // Test Horde bracket selection
    auto hordeBracket = sBotLevelDistribution->SelectBracketWeighted(TEAM_HORDE);
    ASSERT_NE(hordeBracket, nullptr) << "Failed to select Horde bracket";
    EXPECT_GE(hordeBracket->minLevel, 1u);
    EXPECT_LE(hordeBracket->maxLevel, 80u);
    EXPECT_EQ(hordeBracket->faction, TEAM_HORDE);
}

TEST_F(AutomatedWorldPopulationTest, LevelDistribution_BalancesOverTime)
{
    // Select 1000 brackets and verify distribution stays balanced
    std::map<uint32, uint32> allianceCounts;

    for (uint32 i = 0; i < 1000; ++i)
    {
        auto bracket = sBotLevelDistribution->SelectBracketWeighted(TEAM_ALLIANCE);
        ASSERT_NE(bracket, nullptr);
        ++allianceCounts[bracket->minLevel];
    }

    // Verify no single bracket dominates (rough check)
    for (auto const& [minLevel, count] : allianceCounts)
    {
        float percentage = (count / 1000.0f) * 100.0f;
        EXPECT_LT(percentage, 50.0f) << "Bracket L" << minLevel << " has too many bots: " << percentage << "%";
    }
}

// ====================================================================
// GEAR FACTORY TESTS
// ====================================================================

TEST_F(AutomatedWorldPopulationTest, GearFactory_LoadsSuccessfully)
{
    EXPECT_TRUE(sBotGearFactory->IsReady());

    auto stats = sBotGearFactory->GetStats();
    EXPECT_GT(stats.totalItems, 0u) << "No items loaded";
    EXPECT_GT(stats.itemsByClass, 0u) << "No class-specific items";
}

TEST_F(AutomatedWorldPopulationTest, GearFactory_GeneratesCompleteSets)
{
    // Test gear generation for Warrior (Arms spec, level 80)
    auto gearSet = sBotGearFactory->BuildGearSet(CLASS_WARRIOR, 0, 80, TEAM_ALLIANCE);

    EXPECT_TRUE(gearSet.IsComplete()) << "Gear set incomplete";
    EXPECT_GE(gearSet.items.size(), 6u) << "Too few items generated";
    EXPECT_TRUE(gearSet.HasWeapon()) << "No weapon in gear set";
    EXPECT_GT(gearSet.averageIlvl, 0.0f) << "Invalid item level";
}

TEST_F(AutomatedWorldPopulationTest, GearFactory_RespectsLevelProgression)
{
    // Test gear for level 20, 40, 60, 80
    std::vector<uint32> testLevels = {20, 40, 60, 80};

    float previousIlvl = 0.0f;
    for (uint32 level : testLevels)
    {
        auto gearSet = sBotGearFactory->BuildGearSet(CLASS_MAGE, 0, level, TEAM_ALLIANCE);
        EXPECT_GT(gearSet.averageIlvl, previousIlvl)
            << "Item level should increase with character level";
        previousIlvl = gearSet.averageIlvl;
    }
}

TEST_F(AutomatedWorldPopulationTest, GearFactory_AppliesQualityDistribution)
{
    // Generate 100 gear sets and verify quality distribution
    std::map<uint32, uint32> qualityCounts;  // quality -> count
    for (uint32 i = 0; i < 100; ++i)
    {
        auto gearSet = sBotGearFactory->BuildGearSet(CLASS_PALADIN, 0, 80, TEAM_HORDE);
        // Note: Would need to inspect actual item qualities from items
        // This is a placeholder for future enhancement
    }

    // For level 80: Should be ~60% Blue, ~40% Purple
    // Future: Add actual quality inspection
}

// ====================================================================
// TALENT MANAGER TESTS
// ====================================================================

TEST_F(AutomatedWorldPopulationTest, TalentManager_LoadsSuccessfully)
{
    EXPECT_TRUE(sBotTalentManager->IsReady());

    auto stats = sBotTalentManager->GetStats();
    EXPECT_GT(stats.totalLoadouts, 0u) << "No talent loadouts loaded";
}

TEST_F(AutomatedWorldPopulationTest, TalentManager_SelectsValidSpec)
{
    // Test spec selection for all classes
    for (uint8 cls = CLASS_WARRIOR; cls < MAX_CLASSES; ++cls)
    {
        if (cls == CLASS_NONE)
            continue;

        auto specChoice = sBotTalentManager->SelectSpecialization(cls, TEAM_ALLIANCE, 80);
        EXPECT_GT(specChoice.specId, 0u) << "Invalid spec for class " << uint32(cls);
        EXPECT_FALSE(specChoice.specName.empty()) << "Empty spec name";
        EXPECT_GT(specChoice.confidence, 0.0f) << "Invalid confidence";
    }
}

TEST_F(AutomatedWorldPopulationTest, TalentManager_SupportsDualSpec)
{
    EXPECT_FALSE(sBotTalentManager->SupportsDualSpec(9)) << "Dual-spec should not be available at L9";
    EXPECT_TRUE(sBotTalentManager->SupportsDualSpec(10)) << "Dual-spec should be available at L10";
    EXPECT_TRUE(sBotTalentManager->SupportsDualSpec(80)) << "Dual-spec should be available at L80";
}

TEST_F(AutomatedWorldPopulationTest, TalentManager_SupportsHeroTalents)
{
    EXPECT_FALSE(sBotTalentManager->SupportsHeroTalents(70)) << "Hero talents should not be available at L70";
    EXPECT_TRUE(sBotTalentManager->SupportsHeroTalents(71)) << "Hero talents should be available at L71";
    EXPECT_TRUE(sBotTalentManager->SupportsHeroTalents(80)) << "Hero talents should be available at L80";
}

TEST_F(AutomatedWorldPopulationTest, TalentManager_GetsLoadouts)
{
    // Test loadout retrieval for Warrior, level 80
    auto loadout = sBotTalentManager->GetTalentLoadout(CLASS_WARRIOR, 0, 80);

    if (loadout)  // May be nullptr if no loadouts in DB yet
    {
        EXPECT_EQ(loadout->classId, CLASS_WARRIOR);
        EXPECT_EQ(loadout->specId, 0u);
        EXPECT_TRUE(loadout->IsValidForLevel(80));
    }
}

// ====================================================================
// WORLD POSITIONER TESTS
// ====================================================================

TEST_F(AutomatedWorldPopulationTest, WorldPositioner_LoadsSuccessfully)
{
    EXPECT_TRUE(sBotWorldPositioner->IsReady());

    auto stats = sBotWorldPositioner->GetStats();
    EXPECT_GT(stats.totalZones, 0u) << "No zones loaded";
    EXPECT_GT(stats.starterZones, 0u) << "No starter zones";
    EXPECT_GT(stats.levelingZones, 0u) << "No leveling zones";
}

TEST_F(AutomatedWorldPopulationTest, WorldPositioner_SelectsStarterZones)
{
    // Test starter zone selection for L1-4 bots
    auto humanZone = sBotWorldPositioner->SelectZone(1, TEAM_ALLIANCE, RACE_HUMAN);
    EXPECT_TRUE(humanZone.IsValid()) << "Failed to select Human starter zone";
    EXPECT_TRUE(humanZone.placement->isStarterZone) << "Not a starter zone";

    auto orcZone = sBotWorldPositioner->SelectZone(1, TEAM_HORDE, RACE_ORC);
    EXPECT_TRUE(orcZone.IsValid()) << "Failed to select Orc starter zone";
    EXPECT_TRUE(orcZone.placement->isStarterZone) << "Not a starter zone";
}

TEST_F(AutomatedWorldPopulationTest, WorldPositioner_SelectsLevelingZones)
{
    // Test leveling zone selection for L20 bot
    auto zone = sBotWorldPositioner->SelectZone(20, TEAM_ALLIANCE, RACE_HUMAN);
    EXPECT_TRUE(zone.IsValid()) << "Failed to select leveling zone";
    EXPECT_FALSE(zone.placement->isStarterZone) << "Should not be starter zone";
    EXPECT_TRUE(zone.placement->IsValidForLevel(20)) << "Zone not valid for level 20";
}

TEST_F(AutomatedWorldPopulationTest, WorldPositioner_SelectsEndgameZones)
{
    // Test endgame zone selection for L80 bot
    auto zone = sBotWorldPositioner->SelectZone(80, TEAM_ALLIANCE, RACE_HUMAN);
    EXPECT_TRUE(zone.IsValid()) << "Failed to select endgame zone";
    EXPECT_TRUE(zone.placement->IsValidForLevel(80)) << "Zone not valid for level 80";
}

TEST_F(AutomatedWorldPopulationTest, WorldPositioner_ProvidesCapitalFallback)
{
    // Test capital city fallback
    auto capital = sBotWorldPositioner->GetCapitalCity(TEAM_ALLIANCE);
    EXPECT_TRUE(capital.IsValid()) << "Failed to get Alliance capital";

    auto hordeCapital = sBotWorldPositioner->GetCapitalCity(TEAM_HORDE);
    EXPECT_TRUE(hordeCapital.IsValid()) << "Failed to get Horde capital";
}

// ====================================================================
// BOT LEVEL MANAGER TESTS (Orchestrator)
// ====================================================================

TEST_F(AutomatedWorldPopulationTest, BotLevelManager_InitializesSuccessfully)
{
    EXPECT_TRUE(sBotLevelManager->IsReady());
}

TEST_F(AutomatedWorldPopulationTest, BotLevelManager_SelectsLevelBracket)
{
    auto bracket = sBotLevelManager->SelectLevelBracket(TEAM_ALLIANCE);
    EXPECT_NE(bracket, nullptr) << "Failed to select level bracket";
}

TEST_F(AutomatedWorldPopulationTest, BotLevelManager_ConfiguresThrottling)
{
    // Test throttling configuration
    sBotLevelManager->SetMaxBotsPerUpdate(20);
    EXPECT_EQ(sBotLevelManager->GetMaxBotsPerUpdate(), 20u);

    sBotLevelManager->SetMaxBotsPerUpdate(10);  // Reset
}

TEST_F(AutomatedWorldPopulationTest, BotLevelManager_TracksStatistics)
{
    auto stats = sBotLevelManager->GetStats();

    // Initial statistics should be zero
    EXPECT_EQ(stats.totalTasksSubmitted, 0u);
    EXPECT_EQ(stats.totalTasksCompleted, 0u);
    EXPECT_EQ(stats.currentQueueSize, 0u);
}

// ====================================================================
// INTEGRATION TESTS (End-to-End)
// ====================================================================

TEST_F(AutomatedWorldPopulationTest, Integration_AllSystemsCoordinate)
{
    // Verify all systems are ready
    EXPECT_TRUE(sBotLevelDistribution->IsReady());
    EXPECT_TRUE(sBotGearFactory->IsReady());
    EXPECT_TRUE(sBotTalentManager->IsReady());
    EXPECT_TRUE(sBotWorldPositioner->IsReady());
    EXPECT_TRUE(sBotLevelManager->IsReady());

    // Verify systems can communicate
    auto bracket = sBotLevelManager->SelectLevelBracket(TEAM_ALLIANCE);
    ASSERT_NE(bracket, nullptr);

    auto gearSet = sBotGearFactory->BuildGearSet(CLASS_WARRIOR, 0, bracket->maxLevel, TEAM_ALLIANCE);
    EXPECT_TRUE(gearSet.IsComplete());

    auto specChoice = sBotTalentManager->SelectSpecialization(CLASS_WARRIOR, TEAM_ALLIANCE, bracket->maxLevel);
    EXPECT_GT(specChoice.specId, 0u);

    auto zoneChoice = sBotWorldPositioner->SelectZone(bracket->maxLevel, TEAM_ALLIANCE, RACE_HUMAN);
    EXPECT_TRUE(zoneChoice.IsValid());
}

// ====================================================================
// PERFORMANCE TESTS
// ====================================================================

TEST_F(AutomatedWorldPopulationTest, Performance_LevelSelectionIsFast)
{
    auto start = std::chrono::high_resolution_clock::now();

    // Perform 1000 level selections
    for (uint32 i = 0; i < 1000; ++i)
    {
        auto bracket = sBotLevelDistribution->SelectBracketWeighted(TEAM_ALLIANCE);
        ASSERT_NE(bracket, nullptr);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    EXPECT_LT(durationMs, 100) << "1000 selections took " << durationMs << "ms (target: <100ms)";

    float avgMs = durationMs / 1000.0f;
    EXPECT_LT(avgMs, 0.1f) << "Average selection time: " << avgMs << "ms (target: <0.1ms)";
}

TEST_F(AutomatedWorldPopulationTest, Performance_GearGenerationIsFast)
{
    auto start = std::chrono::high_resolution_clock::now();

    // Generate 100 gear sets
    for (uint32 i = 0; i < 100; ++i)
    {
        auto gearSet = sBotGearFactory->BuildGearSet(CLASS_MAGE, 0, 80, TEAM_ALLIANCE);
        ASSERT_TRUE(gearSet.IsComplete());
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    EXPECT_LT(durationMs, 500) << "100 gear sets took " << durationMs << "ms (target: <500ms)";

    float avgMs = durationMs / 100.0f;
    EXPECT_LT(avgMs, 5.0f) << "Average gear generation: " << avgMs << "ms (target: <5ms)";
}

TEST_F(AutomatedWorldPopulationTest, Performance_ZoneSelectionIsFast)
{
    auto start = std::chrono::high_resolution_clock::now();

    // Perform 1000 zone selections
    for (uint32 i = 0; i < 1000; ++i)
    {
        auto zone = sBotWorldPositioner->SelectZone(50, TEAM_ALLIANCE, RACE_HUMAN);
        ASSERT_TRUE(zone.IsValid());
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    EXPECT_LT(durationMs, 50) << "1000 selections took " << durationMs << "ms (target: <50ms)";

    float avgMs = durationMs / 1000.0f;
    EXPECT_LT(avgMs, 0.05f) << "Average zone selection: " << avgMs << "ms (target: <0.05ms)";
}

// ====================================================================
// STRESS TESTS
// ====================================================================

TEST_F(AutomatedWorldPopulationTest, Stress_1000BotsDistribution)
{
    // Simulate 1000 bot creations and verify distribution balance
    std::map<uint32, uint32> levelCounts;

    for (uint32 i = 0; i < 1000; ++i)
    {
        auto bracket = sBotLevelDistribution->SelectBracketWeighted(TEAM_ALLIANCE);
        ASSERT_NE(bracket, nullptr);
        ++levelCounts[bracket->minLevel];
    }

    // Verify distribution is reasonable (no bracket has >30% of bots)
    for (auto const& [minLevel, count] : levelCounts)
    {
        float percentage = (count / 1000.0f) * 100.0f;
        EXPECT_LT(percentage, 30.0f)
            << "Bracket L" << minLevel << " has " << percentage << "% of bots (unbalanced)";
    }
}

// ====================================================================
// MAIN TEST RUNNER
// ====================================================================

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
