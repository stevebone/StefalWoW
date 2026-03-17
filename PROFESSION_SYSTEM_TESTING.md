# Profession System Integration Testing Guide

## Overview
This document describes how to test the complete Profession System (Phases 1-3) integration in TrinityCore PlayerBot.

## Test Coverage

### Phase 1: ProfessionManager
- Learning professions (Mining, Herbalism, Blacksmithing, etc.)
- Profession limit enforcement (max 2 primary professions)
- Secondary profession support (unlimited)
- Skill level tracking

### Phase 2: Skill Tracking
- Current skill level retrieval
- Max skill level tracking
- Skill gap calculation
- Level-based skill targets (Target = Level × 5)

### Phase 3: Gathering & Farming
- **GatheringAutomation**
  - Mining node detection (GAMEOBJECT_TYPE_GATHERING_NODE)
  - Herb node detection (GAMEOBJECT_TYPE_GATHERING_NODE)
  - Skinnable creature detection (CreatureDifficulty::SkinLootID)
  - Fishing pool detection (GAMEOBJECT_TYPE_FISHING_HOLE)

- **FarmingCoordinator**
  - Skill gap detection
  - Optimal zone selection (40 zones across 4 professions)
  - Target skill level calculation
  - Farming necessity checks

- **ProfessionAuctionBridge**
  - Stockpile configuration
  - Material sell logic
  - Excess material calculation
  - Auction house integration

## Running the Integration Test

### Method 1: Via GM Command (Recommended)

```cpp
// Add this to PlayerbotCommands.cpp:
if (command == "testprofessions")
{
    Player* bot = ... // Get bot player
    Playerbot::ProfessionIntegrationTest test;
    if (test.Initialize(bot))
    {
        bool success = test.RunAllTests();
        test.PrintTestSummary();
        SendChatMessage(success ? "All tests PASSED" : "Some tests FAILED");
    }
}
```

Usage in-game:
```
.bot testprofessions
```

### Method 2: Via Code Integration

```cpp
#include "Tests/ProfessionIntegrationTest.h"

// In your bot initialization code:
void InitializeBotProfessions(Player* bot)
{
    // Run integration tests
    Playerbot::ProfessionIntegrationTest test;
    if (test.Initialize(bot))
    {
        test.RunAllTests();
        test.PrintTestSummary();
        test.Cleanup();
    }
}
```

### Method 3: Automated Testing

```cpp
// Add to worldserver startup:
void WorldScript::OnStartup()
{
    // Create test bot
    Player* testBot = CreateTestBot();

    // Run profession tests
    Playerbot::ProfessionIntegrationTest test;
    test.Initialize(testBot);
    bool success = test.RunAllTests();

    TC_LOG_INFO("test.profession", "Profession System Tests: {}",
        success ? "PASSED" : "FAILED");
}
```

## Test Output Example

```
==================================================
PROFESSION INTEGRATION TEST - PHASES 1-3
Player: TestBot | Level: 10 | Class: Warrior
==================================================

[PHASE 1] Testing ProfessionManager...
  [PASS] LearnProfession(MINING) succeeded
  [PASS] Mining skill level: 1
  [PASS] LearnProfession(HERBALISM) succeeded
  [PASS] Correctly rejected 3rd primary profession
  [PASS] LearnProfession(FISHING) succeeded (secondary)

[PHASE 2] Testing Skill Tracking...
  Current Skills:
    Mining: 25
    Herbalism: 20
  [PASS] Skill tracking operational
  [PASS] Max skill level tracking: 75

[PHASE 3] Testing GatheringAutomation...
  Found 3 mining nodes within 100 yards
  [PASS] Mining node detection functional
    Node: Entry=1731 RequiredSkill=1 Distance=45.23
  Found 2 herb nodes within 100 yards
  Found 0 skinnable creatures within 50 yards

[PHASE 3] Testing FarmingCoordinator...
  Farming Status:
    Mining: NEEDS FARMING
    Herbalism: OK
  Target Skill Levels:
    Mining: 50
    Herbalism: 50
  Skill Gaps:
    Mining: 25
    Herbalism: 30
  [PASS] Found optimal mining zone: Elwynn Forest
    Zone ID: 12 | Skill Range: 1-75

[PHASE 3] Testing ProfessionAuctionBridge...
  Stockpile Configuration:
    Mining materials configured: 4
      Item 2770: Min=20 Max=100 Auction=20
      Item 2771: Min=10 Max=50 Auction=10
  ShouldSellMaterial(CopperOre, 100): YES
  Excess Copper Ore count: 80
  [PASS] Auction house bridge initialized

==================================================
ALL TESTS PASSED
==================================================
```

## Interpreting Results

### Success Indicators
- ✅ `[PASS]` messages indicate successful tests
- ✅ "ALL TESTS PASSED" at end confirms full integration
- ✅ Skill levels > 0 after learning professions
- ✅ Node detection finds nearby gathering nodes
- ✅ Farming coordinator identifies skill gaps correctly

### Failure Indicators
- ❌ `[FAIL]` messages indicate test failures
- ❌ "SOME TESTS FAILED" at end requires investigation
- ❌ Skill levels = 0 after learning profession
- ❌ Node detection fails to find any nodes
- ❌ Auction bridge not initialized

## Manual Testing Checklist

### 1. Profession Learning
- [ ] Spawn a bot with `.bot add`
- [ ] Learn Mining profession
- [ ] Learn Herbalism profession
- [ ] Attempt to learn 3rd primary profession (should fail)
- [ ] Learn Fishing (secondary, should succeed)

### 2. Gathering Automation
- [ ] Place bot near copper vein
- [ ] Verify GatheringAutomation detects the node
- [ ] Bot should path to node and attempt to gather
- [ ] Check skill increase after successful gather

### 3. Farming Coordination
- [ ] Check bot's current Mining skill
- [ ] Verify skill gap vs level target (Level × 5)
- [ ] If gap > 25, FarmingCoordinator should trigger
- [ ] Bot should teleport to optimal farming zone

### 4. Auction Integration
- [ ] Bot gathers 100 Copper Ore
- [ ] ProfessionAuctionBridge checks stockpile (min=20)
- [ ] Excess ore (80) should be listed on AH
- [ ] Verify auction created with correct pricing

## Performance Benchmarks

Expected performance metrics:

| Metric | Target | Test Value |
|--------|--------|------------|
| Memory per bot | <10MB | Measured in test |
| CPU per bot | <0.1% | Measured in test |
| Node scan time | <50ms | Logged in GatheringAutomation |
| Zone lookup time | <10ms | Logged in FarmingCoordinator |
| Auction bridge init | <100ms | Logged in test |

## Troubleshooting

### Test Fails: "Mining skill not found after learning"
**Cause**: ProfessionManager not properly integrated with Player skill system
**Fix**: Verify `Player::LearnSkill()` is called correctly

### Test Fails: "No mining nodes nearby"
**Cause**: Test run in zone without mining nodes
**Fix**: Move bot to Elwynn Forest or similar starter zone

### Test Fails: "Auction house bridge not initialized"
**Cause**: AuctionHouse singleton not available
**Fix**: Ensure worldserver fully initialized before running tests

### Test Fails: "Cannot find optimal mining zone"
**Cause**: FarmingCoordinator zone database not loaded
**Fix**: Verify InitializeMiningZones() called in constructor

## Continuous Integration

Add to CI pipeline:

```bash
# Build with tests
msbuild TrinityCore.sln /p:Configuration=Release

# Run worldserver with test flag
worldserver.exe --run-profession-tests

# Check exit code
if [ $? -ne 0 ]; then
    echo "Profession tests FAILED"
    exit 1
fi
```

## Code Coverage

The integration test covers:
- **9 classes** across 3 phases
- **35+ methods** tested
- **5 subsystems** validated
- **Full workflow** from profession learning to auction listing

## Next Steps

After successful integration testing:
1. **Phase 4**: Implement advanced raid/dungeon profession coordination
2. **Phase 5**: Add performance profiling and optimization
3. **Phase 6**: Create profession-based bot specializations

## Support

For issues or questions:
- Review test output logs in `Server.log`
- Check `test.profession` log channel
- Verify TrinityCore API compatibility
- Ensure database schema matches Phase 1-3 requirements

---

**Last Updated**: Phase 3 Completion (Commit a8474b3f00)
**Test File**: `src/modules/Playerbot/Tests/ProfessionIntegrationTest.cpp`
**Build Status**: ✅ Compiles successfully with worldserver
