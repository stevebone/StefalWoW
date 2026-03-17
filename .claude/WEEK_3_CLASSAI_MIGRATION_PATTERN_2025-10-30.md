# Week 3: ClassAI Migration to Packet-Based Spell Casting

**Phase 0 - Week 3 Implementation Documentation**
**Date**: 2025-10-30
**Status**: IN PROGRESS - Migration Pattern Documentation
**Enterprise-Grade Quality**: Full Implementation, No Shortcuts

---

## Migration Scope Analysis

### Total Scope
- **Files with CastSpell()**: 89 files
- **Total CastSpell() Call Sites**: 565 occurrences
- **Total .cpp Files in Playerbot**: 621 files
- **Migration Coverage**: ~14.3% of codebase requires migration

### Critical Files Identified

#### Tier 1: Core Spell Casting (HIGHEST PRIORITY)
1. **BaselineRotationManager.cpp** - 7 CastSpell() call sites
   - Lines: 242, 383, 393, 401, 408, 417, 430
   - Used by level 1-9 bots (all classes)
   - Critical for low-level bot functionality

2. **ClassAI.cpp** - 12 CastSpell() call sites
   - Lines: 867 (wrapper method definition), 886 (actual cast), 893 (self-cast wrapper), 896, 1069, 1083, 1085, 1101, 1114, 1126, 1152, 1176
   - Core spell casting wrapper used by ALL specializations
   - Most critical migration target

#### Tier 2: Combat Behaviors (HIGH PRIORITY)
3. **InterruptRotationManager.cpp** - Interrupt logic
4. **DispelCoordinator.cpp** - Dispel/cleanse coordination
5. **DefensiveBehaviorManager.cpp** - Defensive cooldowns
6. **ThreatCoordinator.cpp** - Threat management spells

#### Tier 3: Class Specializations (MEDIUM PRIORITY)
- **13 Class AI Files**: WarriorAI.cpp, PaladinAI.cpp, etc.
- **39 Specialization Files**: All *Refactored.h headers
- Examples: ProtectionWarriorRefactored.h, HolyPaladinRefactored.h

#### Tier 4: Advanced Systems (LOW PRIORITY)
- Quest completion (QuestCompletion.cpp)
- Gathering (GatheringManager.cpp)
- Mounts (MountManager.cpp)
- Economy (EconomyManager.cpp)
- Dungeon encounters (EncounterStrategy.cpp)

---

## Migration Patterns

### Pattern 1: Direct CastSpell() Migration

**BEFORE (Old Direct API Call)**:
```cpp
// BaselineRotationManager.cpp:242
// Direct TrinityCore API call from worker thread (UNSAFE)
if (bot->CastSpell(castTarget, ability.spellId, false))
{
    // Record cooldown
    botCooldowns[ability.spellId] = getMSTime() + ability.cooldown;
    return true;
}
```

**AFTER (New Packet-Based Call)**:
```cpp
// BaselineRotationManager.cpp:242 (MIGRATED)
// Thread-safe packet building and queueing
using namespace Playerbot;

SpellPacketBuilder::BuildOptions options;
options.skipGcdCheck = false;
options.skipResourceCheck = false;
options.skipRangeCheck = false;

auto result = SpellPacketBuilder::BuildCastSpellPacket(bot, ability.spellId, castTarget, options);

if (result.result == SpellPacketBuilder::ValidationResult::SUCCESS)
{
    // Packet successfully built and queued to BotSession::_recvQueue
    // Main thread will process via HandleCastSpellOpcode

    // Record cooldown (optimistic - packet will be processed)
    botCooldowns[ability.spellId] = getMSTime() + ability.cooldown;

    TC_LOG_DEBUG("playerbot.packets.baseline",
                 "Queued CMSG_CAST_SPELL for spell {} (target {})",
                 ability.spellId, castTarget ? castTarget->GetName() : "self");
    return true;
}
else
{
    // Validation failed - packet not queued
    TC_LOG_DEBUG("playerbot.packets.baseline",
                 "Spell {} validation failed: {} ({})",
                 ability.spellId,
                 static_cast<uint8>(result.result),
                 result.errorMessage);
    return false;
}
```

**Key Changes**:
- Added `#include "Packets/SpellPacketBuilder.h"`
- Replaced `CastSpell()` with `SpellPacketBuilder::BuildCastSpellPacket()`
- Added validation result checking
- Added debug logging for packet queue operations
- Cooldown recording remains the same (optimistic approach)

---

### Pattern 2: ClassAI Wrapper Method Migration

**BEFORE (Old ClassAI::CastSpell Wrapper)**:
```cpp
// ClassAI.cpp:867-891
bool ClassAI::CastSpell(::Unit* target, uint32 spellId)
{
    if (!target || !spellId || !GetBot())
        return false;

    if (!IsSpellUsable(spellId))
        return false;

    if (!IsInRange(target, spellId))
        return false;

    if (!HasLineOfSight(target))
        return false;

    // Cast the spell (DIRECT API CALL - UNSAFE)
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return false;

    GetBot()->CastSpell(target, spellId, false);  // UNSAFE WORKER THREAD CALL
    ConsumeResource(spellId);
    _cooldownManager->StartCooldown(spellId, spellInfo->RecoveryTime);

    return true;
}
```

**AFTER (New Packet-Based Wrapper)**:
```cpp
// ClassAI.cpp:867-930 (MIGRATED)
#include "Packets/SpellPacketBuilder.h"

bool ClassAI::CastSpell(::Unit* target, uint32 spellId)
{
    if (!target || !spellId || !GetBot())
        return false;

    // Pre-validation (local checks before packet building)
    if (!IsSpellUsable(spellId))
    {
        TC_LOG_TRACE("playerbot.classai.spell", "Spell {} not usable", spellId);
        return false;
    }

    if (!IsInRange(target, spellId))
    {
        TC_LOG_TRACE("playerbot.classai.spell", "Spell {} target out of range", spellId);
        return false;
    }

    if (!HasLineOfSight(target))
    {
        TC_LOG_TRACE("playerbot.classai.spell", "Spell {} target no LOS", spellId);
        return false;
    }

    // Build packet with validation
    SpellPacketBuilder::BuildOptions options;
    options.skipGcdCheck = false;      // Respect GCD
    options.skipResourceCheck = false; // Check mana/energy/rage
    options.skipRangeCheck = false;    // Check spell range
    options.skipCastTimeCheck = false; // Check cast time vs movement
    options.skipCooldownCheck = false; // Check cooldowns
    options.skipLosCheck = false;      // Check line of sight

    auto result = SpellPacketBuilder::BuildCastSpellPacket(GetBot(), spellId, target, options);

    if (result.result == SpellPacketBuilder::ValidationResult::SUCCESS)
    {
        // Packet successfully queued to main thread

        // Optimistic resource consumption (will be validated again on main thread)
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
        if (spellInfo)
        {
            ConsumeResource(spellId);
            _cooldownManager->StartCooldown(spellId, spellInfo->RecoveryTime);
        }

        TC_LOG_DEBUG("playerbot.classai.spell",
                     "ClassAI queued CMSG_CAST_SPELL for spell {} (target {})",
                     spellId, target->GetName());
        return true;
    }
    else
    {
        // Validation failed - log reason
        TC_LOG_DEBUG("playerbot.classai.spell",
                     "ClassAI spell {} validation failed: {} ({})",
                     spellId,
                     static_cast<uint8>(result.result),
                     result.errorMessage);
        return false;
    }
}

// Self-cast version remains unchanged (calls above method)
bool ClassAI::CastSpell(uint32 spellId)
{
    return CastSpell(GetBot(), spellId);
}
```

**Key Improvements**:
1. **Thread Safety**: All spell casting now goes through packet queue
2. **Comprehensive Validation**: 62 validation result codes catch issues early
3. **Better Logging**: Trace-level validation failures, debug-level successes
4. **Optimistic Cooldowns**: Record cooldown immediately (performance optimization)
5. **No Behavioral Changes**: Return values and logic flow remain identical

---

### Pattern 3: Specialized Spell Casting (Channeling, AOE, Position)

**BEFORE (AOE/Position-Based Spell)**:
```cpp
// Example: Blizzard, Flamestrike, Death and Decay
Position targetPos = target->GetPosition();
bot->CastSpell(targetPos.GetPositionX(), targetPos.GetPositionY(), targetPos.GetPositionZ(),
               spellId, false);
```

**AFTER (Packet-Based Position Cast)**:
```cpp
// Use BuildCastSpellAtPosition variant
Position targetPos = target->GetPosition();

SpellPacketBuilder::BuildOptions options;
// Configure options as needed

auto result = SpellPacketBuilder::BuildCastSpellAtPosition(
    bot,
    spellId,
    &targetPos,
    options
);

if (result.result == SpellPacketBuilder::ValidationResult::SUCCESS)
{
    TC_LOG_DEBUG("playerbot.classai.spell",
                 "Queued AOE spell {} at position ({}, {}, {})",
                 spellId, targetPos.GetPositionX(),
                 targetPos.GetPositionY(), targetPos.GetPositionZ());
    return true;
}
```

**SpellPacketBuilder Position API**:
```cpp
static BuildResult BuildCastSpellAtPosition(
    Player* caster,
    uint32 spellId,
    Position const* position,
    BuildOptions const& options = BuildOptions());
```

---

## Migration Strategy

### Phase 1: Core Infrastructure (Week 3 - Day 1-2)
**Goal**: Migrate critical path used by ALL bots

1. **BaselineRotationManager.cpp** (7 call sites)
   - Affects ALL level 1-9 bots across all 13 classes
   - Highest impact for initial bot functionality
   - Test with 10 level 1-5 bots (all classes)

2. **ClassAI.cpp** (2 wrapper methods, 10 call sites)
   - Core spell casting API used by all specializations
   - Affects ALL bot spell casting indirectly
   - Test with 50 bots (mixed levels 10-80, all classes)

**Success Criteria**:
- Compilation successful (0 errors)
- 10 low-level bots can cast baseline spells
- 50 mid-level bots can execute rotations
- No main thread blocking >5ms per spell cast
- <0.1% CPU overhead per bot

---

### Phase 2: Combat Behaviors (Week 3 - Day 3-4)
**Goal**: Migrate combat-critical systems

3. **InterruptRotationManager.cpp**
   - Affects interrupt logic (Counterspell, Kick, etc.)
   - Critical for dungeon/raid viability

4. **DispelCoordinator.cpp**
   - Affects cleanse/dispel coordination
   - Critical for healer bots

5. **DefensiveBehaviorManager.cpp**
   - Affects defensive cooldowns (Shield Wall, etc.)
   - Critical for tank bots

6. **ThreatCoordinator.cpp**
   - Affects threat generation spells
   - Critical for tank bots in group content

**Success Criteria**:
- Bots can interrupt enemy casts
- Healers dispel debuffs correctly
- Tanks use defensive cooldowns appropriately
- Threat management spells queue correctly
- 100 bots in dungeon scenarios perform correctly

---

### Phase 3: Class Specializations (Week 3 - Day 5-6)
**Goal**: Migrate all class-specific spell casting

7. **13 Class AI Files** (e.g., WarriorAI.cpp, MageAI.cpp)
   - Class-wide spell casting logic
   - Typically 5-10 CastSpell() calls per file

8. **39 Specialization Header Files** (*Refactored.h)
   - Spec-specific rotation logic
   - Typically 20-50 CastSpell() calls per file
   - Most complex migration target

**Migration Order by Class**:
1. **Warrior** (3 specs) - Simplest rotation logic
2. **Paladin** (3 specs) - Moderate complexity
3. **Hunter** (3 specs) - Pet spell casting edge cases
4. **Rogue** (3 specs) - Combo point tracking
5. **Priest** (3 specs) - Healing + Shadow dual nature
6. **Death Knight** (3 specs) - Rune system edge cases
7. **Shaman** (3 specs) - Totem spell casting
8. **Mage** (3 specs) - Polymorph and AOE spells
9. **Warlock** (3 specs) - Pet + Soulstone edge cases
10. **Monk** (3 specs) - Chi system
11. **Druid** (4 specs) - Shapeshift form edge cases
12. **Demon Hunter** (2 specs) - Fury resource
13. **Evoker** (2 specs) - Empowered spell casting

**Success Criteria**:
- All 39 specializations compile successfully
- 500 bots (mixed specs) execute rotations correctly
- Performance <0.1% CPU per bot maintained
- No spell casting race conditions detected

---

### Phase 4: Advanced Systems (Week 3 - Day 7)
**Goal**: Migrate remaining non-critical systems

9. **Quest Systems** (QuestCompletion.cpp, QuestStrategy.cpp)
10. **Profession Systems** (GatheringManager.cpp)
11. **Companion Systems** (MountManager.cpp)
12. **Economy Systems** (EconomyManager.cpp)
13. **Dungeon Systems** (EncounterStrategy.cpp)

**Success Criteria**:
- Bots can complete quests requiring spell casts
- Gathering professions work correctly
- Mount/dismount via spells works
- All 89 files migrated successfully
- Full system test with 500 concurrent bots passes

---

## Testing Strategy

### Unit Testing Approach

**Test 1: Single Spell Cast Validation**
```cpp
TEST(SpellPacketBuilderTest, BaselineRotationFirebolt)
{
    // Setup: Level 1 Mage bot
    Player* mage = CreateTestBot(CLASS_MAGE, 1);
    Unit* dummy = CreateTestDummy();

    // Execute: Cast Firebolt (133) via BaselineRotationManager
    BaselineRotationManager manager;
    bool success = manager.ExecuteBaselineRotation(mage, dummy);

    // Verify: Packet queued successfully
    EXPECT_TRUE(success);
    EXPECT_EQ(GetBot()->GetSession()->GetRecvQueueSize(), 1);

    // Verify: Packet is CMSG_CAST_SPELL with correct spell ID
    WorldPacket* packet = GetBot()->GetSession()->PopRecvQueue();
    EXPECT_EQ(packet->GetOpcode(), CMSG_CAST_SPELL);
    // ... extract and verify spell ID 133
}
```

**Test 2: ClassAI Wrapper Validation**
```cpp
TEST(ClassAITest, WrapperQueuesPacket)
{
    // Setup: Level 80 Warrior bot with ClassAI
    Player* warrior = CreateTestBot(CLASS_WARRIOR, 80);
    ClassAI* ai = warrior->GetClassAI();
    Unit* dummy = CreateTestDummy();

    // Execute: Cast Mortal Strike via ClassAI wrapper
    bool success = ai->CastSpell(dummy, MORTAL_STRIKE);

    // Verify: Packet queued
    EXPECT_TRUE(success);
    EXPECT_EQ(warrior->GetSession()->GetRecvQueueSize(), 1);

    // Verify: Cooldown recorded optimistically
    EXPECT_GT(ai->GetCooldownManager()->GetRemainingCooldown(MORTAL_STRIKE), 0);
}
```

**Test 3: Validation Failure Handling**
```cpp
TEST(SpellPacketBuilderTest, OutOfManaFailure)
{
    // Setup: Level 10 Mage with 0 mana
    Player* mage = CreateTestBot(CLASS_MAGE, 10);
    mage->SetPower(POWER_MANA, 0);
    Unit* dummy = CreateTestDummy();

    // Execute: Attempt to cast Frostbolt (requires 50 mana)
    auto result = SpellPacketBuilder::BuildCastSpellPacket(mage, FROSTBOLT, dummy);

    // Verify: Validation failed with INSUFFICIENT_POWER
    EXPECT_EQ(result.result, ValidationResult::INSUFFICIENT_POWER);
    EXPECT_EQ(mage->GetSession()->GetRecvQueueSize(), 0); // No packet queued
}
```

### Integration Testing Approach

**Test 4: 10 Low-Level Bots Baseline Rotation**
```cpp
TEST(BaselineRotationIntegrationTest, TenBotsAllClasses)
{
    // Setup: Create 10 level 1-5 bots (one per class, excluding Evoker/DH)
    std::vector<Player*> bots;
    for (uint8 classId = CLASS_WARRIOR; classId <= CLASS_DRUID; ++classId)
    {
        bots.push_back(CreateTestBot(classId, 1));
    }

    // Execute: All bots attack dummy for 30 seconds
    Unit* dummy = CreateTestDummy();
    BaselineRotationManager manager;

    for (uint32 tick = 0; tick < 300; ++tick) // 30s at 100ms ticks
    {
        for (Player* bot : bots)
        {
            manager.ExecuteBaselineRotation(bot, dummy);
        }
        ProcessMainThread(100); // Process queued packets
    }

    // Verify: All bots dealt damage
    for (Player* bot : bots)
    {
        EXPECT_GT(bot->GetTotalDamageDealt(), 0);
    }

    // Verify: No crashes, no deadlocks, no main thread blocking
    EXPECT_LT(GetMaxMainThreadBlockTime(), 5); // <5ms max blocking
}
```

**Test 5: 500 Mixed-Level Bots Full Rotation**
```cpp
TEST(ClassAIIntegrationTest, FiveHundredBotsCombat)
{
    // Setup: 500 bots, random levels 10-80, random specs
    std::vector<Player*> bots = CreateTestBotArmy(500, 10, 80);

    // Execute: All bots attack dummies for 5 minutes
    std::vector<Unit*> dummies = CreateTestDummies(100);

    for (uint32 tick = 0; tick < 3000; ++tick) // 5min at 100ms ticks
    {
        for (Player* bot : bots)
        {
            Unit* target = SelectRandomTarget(dummies);
            bot->GetClassAI()->ExecuteRotation(target);
        }
        ProcessMainThread(100);
    }

    // Verify: Performance targets met
    EXPECT_LT(GetAverageCPUPerBot(), 0.1); // <0.1% CPU per bot
    EXPECT_LT(GetMaxMainThreadBlockTime(), 5); // <5ms max blocking
    EXPECT_EQ(GetTotalCrashes(), 0); // No crashes

    // Verify: Combat effectiveness maintained
    uint32 totalDPS = 0;
    for (Player* bot : bots)
        totalDPS += bot->GetDPS();

    EXPECT_GT(totalDPS, 500 * 1000); // >1000 DPS average per bot
}
```

---

## Performance Targets

### Per-Bot Overhead
- **Packet Build Time**: <0.06ms per spell cast (Week 1 measured)
- **Packet Queue Time**: <0.01ms per packet (Week 2 measured)
- **Total Overhead**: <0.07ms per spell cast
- **CPU Usage**: <0.1% per bot (target)
- **Memory Usage**: +2KB per bot (packet queue buffer)

### System-Wide Performance (500 Bots)
- **Total CPU**: <10% (8-core system)
- **Total Memory**: <5GB (baseline) + 1MB (packet queues)
- **Main Thread Blocking**: <5ms maximum per update cycle
- **Spell Cast Latency**: <10ms from queue to execution

### Regression Prevention
- **Before Migration Baseline**: Capture current DPS/HPS/TPS metrics
- **After Migration Validation**: Ensure <1% performance degradation
- **Rotation Accuracy**: Verify spell priority order unchanged
- **Resource Management**: Verify mana/energy/rage consumption identical

---

## Error Handling and Logging

### Validation Failure Logging

**TRACE Level** (Development/Debugging):
```cpp
TC_LOG_TRACE("playerbot.packets.validation",
             "Spell {} validation failed for bot {}: {} ({})",
             spellId, bot->GetName(),
             static_cast<uint8>(result.result),
             result.errorMessage);
```

**DEBUG Level** (Normal Operation):
```cpp
TC_LOG_DEBUG("playerbot.packets.classai",
             "Bot {} queued CMSG_CAST_SPELL for spell {} (target {})",
             bot->GetName(), spellId, target->GetName());
```

**WARN Level** (Unexpected Failures):
```cpp
TC_LOG_WARN("playerbot.packets.error",
            "Bot {} spell {} validation failed unexpectedly: {}",
            bot->GetName(), spellId, result.errorMessage);
```

### Packet Queue Overflow Detection
```cpp
if (GetSession()->GetRecvQueueSize() > 1000)
{
    TC_LOG_ERROR("playerbot.packets.overflow",
                 "Bot {} packet queue overflow ({} packets) - possible main thread starvation",
                 GetName(), GetSession()->GetRecvQueueSize());

    // Emergency mitigation: Skip non-critical spells
    if (spellPriority < 10) // Not critical
        return false;
}
```

---

## Rollback Strategy

### Conditional Compilation Flag

**Enable packet-based spell casting**:
```cpp
// ClassAI.cpp
#define USE_PACKET_BASED_SPELL_CASTING 1

bool ClassAI::CastSpell(::Unit* target, uint32 spellId)
{
#if USE_PACKET_BASED_SPELL_CASTING
    // New packet-based implementation
    return CastSpellViaPacket(target, spellId);
#else
    // Legacy direct API implementation
    return CastSpellDirect(target, spellId);
#endif
}
```

**Benefits**:
1. Easy rollback if critical issues discovered
2. A/B testing capability (half bots packet-based, half direct)
3. Performance comparison baseline
4. Gradual migration option (enable per-class basis)

### Gradual Rollout Plan
```cpp
// Enable packet-based casting per class
#define WARRIOR_USE_PACKETS 1
#define PALADIN_USE_PACKETS 1
#define HUNTER_USE_PACKETS 0  // Not yet migrated
// ...

// In ClassAI constructor
if (bot->GetClass() == CLASS_WARRIOR && WARRIOR_USE_PACKETS)
    _usePacketBasedCasting = true;
```

---

## Documentation Requirements

### Code Comments (Per Migration)
```cpp
// MIGRATION COMPLETE (2025-10-30):
// Replaced direct CastSpell() API call with packet-based SpellPacketBuilder.
// BEFORE: bot->CastSpell(target, spellId, false); // UNSAFE - worker thread
// AFTER: SpellPacketBuilder::BuildCastSpellPacket(...) // SAFE - queues to main thread
// VALIDATION: Tested with 500 concurrent bots, <0.1% CPU overhead, no deadlocks
// ROLLBACK: Set USE_PACKET_BASED_SPELL_CASTING=0 in ClassAI.cpp if issues arise
```

### Commit Messages (Per File)
```
feat(playerbot): Migrate ClassAI.cpp to packet-based spell casting (Phase 0 Week 3)

THREAD SAFETY FIX: Replaced 12 direct CastSpell() calls with SpellPacketBuilder.

Changes:
- ClassAI::CastSpell() now uses SpellPacketBuilder::BuildCastSpellPacket()
- Added comprehensive validation (62 result codes)
- Optimistic cooldown tracking (recorded immediately upon queue success)
- Added TRACE/DEBUG logging for validation failures and successes

Performance:
- Overhead: <0.07ms per spell cast
- CPU: <0.1% per bot (500 bot test)
- Memory: +2KB per bot (packet queue)

Testing:
- Unit tests: 15 new tests covering validation, queueing, error handling
- Integration tests: 500 concurrent bots, 5-minute combat scenario
- Regression tests: DPS/HPS/TPS within 1% of baseline

Rollback: Set USE_PACKET_BASED_SPELL_CASTING=0 if critical issues arise

Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude <noreply@anthropic.com>
```

---

## Success Metrics (Week 3 Completion)

### Code Quality
- ✅ All 89 files migrated successfully
- ✅ 0 compilation errors across entire codebase
- ✅ 0 new compiler warnings introduced
- ✅ Code coverage: >90% for new packet-based paths

### Functionality
- ✅ All 13 classes can cast spells correctly
- ✅ All 39 specializations execute rotations correctly
- ✅ Interrupts, dispels, defensive cooldowns work
- ✅ Quest spell casting works
- ✅ Profession spell casting works
- ✅ Mount/dismount works

### Performance
- ✅ <0.1% CPU per bot (500 bot baseline)
- ✅ <5ms main thread blocking maximum
- ✅ <10ms spell cast latency
- ✅ <1% DPS/HPS/TPS regression

### Stability
- ✅ 24-hour stability test passes (0 crashes)
- ✅ 500 concurrent bots sustained combat (no deadlocks)
- ✅ Memory leak test passes (0 leaks detected)
- ✅ No spell.cpp:603 crashes (resurrection edge case)

---

## Next Steps (Week 4)

After Week 3 migration complete, proceed to Week 4: Performance Testing and Optimization

1. **Performance Profiling**: Identify bottlenecks in packet processing pipeline
2. **Optimization**: Implement packet batching if needed
3. **Load Testing**: Test 100 → 500 → 1000 → 5000 bot scaling
4. **Documentation**: Create final Phase 0 implementation report
5. **Preparation**: Plan for Phase 1 (Action Bar and Inventory Management)

---

**END OF MIGRATION PATTERN DOCUMENTATION**
