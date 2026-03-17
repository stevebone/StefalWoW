# Phase 2.3: Fix Combat Activation - Universal ClassAI

**Duration**: 1 week (2025-01-27 to 2025-02-03)
**Status**: ⏳ PENDING
**Owner**: Development Team

---

## Objectives

Make ClassAI (class-specific combat AI) work in ALL combat situations, not just group combat:
1. Remove group-only restriction from OnCombatUpdate()
2. Ensure ClassAI activates during solo questing
3. Ensure ClassAI activates when gathering mobs interrupt
4. Maintain ClassAI activation in group/dungeon/raid scenarios
5. Verify all 13 classes work correctly

---

## Background

### Current Problem
```cpp
// CURRENT (WRONG) - Only runs in group combat
void BotAI::UpdateAI(uint32 diff)
{
    UpdateStrategies(diff);

    if (IsInCombat() && HasStrategy("group_combat"))  // ❌ Group-only restriction
    {
        OnCombatUpdate(diff);  // ClassAI called here
    }
}
```

**Result**:
- ✅ Bot in dungeon group → ClassAI works, spells cast
- ❌ Bot questing solo → ClassAI never called, only auto-attacks
- ❌ Bot gathering, mob attacks → ClassAI never called, only auto-attacks

### Root Cause
OnCombatUpdate() (which calls ClassAI) is gated behind group combat strategy check. This was intended for group coordination but accidentally disabled ALL spell casting for solo bots.

### Solution
```cpp
// NEW (CORRECT) - Runs in ALL combat
void BotAI::UpdateAI(uint32 diff)
{
    UpdateStrategies(diff);

    if (IsInCombat())  // ✅ No group check - always run
    {
        OnCombatUpdate(diff);  // ClassAI runs for EVERYONE in combat
    }
}
```

**Result**:
- ✅ Bot in dungeon group → ClassAI works
- ✅ Bot questing solo → ClassAI works, casts spells
- ✅ Bot gathering, interrupted → ClassAI works, fights back properly

---

## Technical Requirements

### Performance Constraints
- OnCombatUpdate() must remain <1ms per bot
- ClassAI::Update() must remain <0.5ms per bot
- Total combat overhead <1.5ms per bot
- Must scale to 500+ bots in simultaneous combat

### ClassAI Architecture
Each class has specialized ClassAI:
- WarriorClassAI - Rage management, stance dancing
- PaladinClassAI - Seal management, holy power
- HunterClassAI - Pet control, focus management
- RogueClassAI - Combo points, energy management
- PriestClassAI - Mana management, dual-spec healing/shadow
- ShamanClassAI - Totem management, shock/lightning rotation
- MageClassAI - Arcane/Fire/Frost rotations, mana gems
- WarlockClassAI - Pet management, soul shards, DoTs
- DruidClassAI - Shapeshifting, HoTs, versatile forms
- DeathKnightClassAI - Rune management, presence system
- MonkClassAI - Chi/energy management, roll mechanics
- DemonHunterClassAI - Fury/pain resource, metamorphosis
- EvokerClassAI - Essence management, empowerment system

### Integration Points
- BotAI::OnCombatUpdate() is the single entry point
- OnCombatUpdate() calls _classAI->Update(diff)
- ClassAI has full access to Player* for spell casting
- Must work with CombatMovementStrategy (Phase 2.2)

---

## Deliverables

### 1. Modified BotAI::UpdateAI()
Location: `src/modules/Playerbot/AI/BotAI.cpp`

**Before** (lines ~140-160):
```cpp
void BotAI::UpdateAI(uint32 diff)
{
    if (!_bot || !_bot->IsInWorld())
        return;

    // Update active strategies
    UpdateStrategies(diff);

    // Update movement
    UpdateMovement(diff);

    // Combat specialization (GROUP COMBAT ONLY) ❌
    if (IsInCombat() && HasActiveStrategy("group_combat"))
    {
        OnCombatUpdate(diff);
    }
}
```

**After**:
```cpp
void BotAI::UpdateAI(uint32 diff)
{
    if (!_bot || !_bot->IsInWorld())
        return;

    // PHASE 1: Manager updates (heavyweight, throttled)
    UpdateManagers(diff);  // From Phase 2.4

    // PHASE 2: Strategy updates (lightweight, every frame)
    UpdateStrategies(diff);

    // PHASE 3: Movement execution
    UpdateMovement(diff);

    // PHASE 4: Combat specialization (ALL COMBAT) ✅
    if (IsInCombat())  // No group restriction!
    {
        OnCombatUpdate(diff);
    }
}
```

### 2. Verify ClassAI Initialization
Location: `src/modules/Playerbot/AI/BotAI.cpp`

**Ensure ClassAI is created for ALL bots**:
```cpp
void BotAI::Initialize()
{
    if (!_bot)
        return;

    // Create class-specific AI
    _classAI = BotAIFactory::CreateClassAI(_bot, this);
    if (!_classAI)
    {
        TC_LOG_ERROR("module.playerbot", "Failed to create ClassAI for bot {}", _bot->GetName());
        return;
    }

    // Initialize strategies
    InitializeStrategies();

    TC_LOG_INFO("module.playerbot", "BotAI initialized for {} (class: {})",
                _bot->GetName(), _bot->getClass());
}
```

### 3. Test Solo Quest Combat
Location: `tests/integration/SoloQuestCombatTest.cpp`

**Test Scenario**:
```cpp
TEST(SoloQuestCombat, WarriorCastsSpells)
{
    // Setup: Create level 20 Warrior bot, no group
    Bot* warrior = CreateTestBot(CLASS_WARRIOR, 20);
    ASSERT_FALSE(warrior->GetGroup());

    // Action: Bot attacks training dummy
    Unit* dummy = SpawnTrainingDummy();
    warrior->Attack(dummy, true);

    // Wait for combat
    WaitForCombat(warrior, 5000);
    ASSERT_TRUE(warrior->IsInCombat());

    // Verify: Bot casts Heroic Strike, not just auto-attacks
    uint32 heroicStrikeCount = CountSpellCasts(warrior, 78); // Heroic Strike
    ASSERT_GT(heroicStrikeCount, 0) << "Warrior should cast Heroic Strike in solo combat";

    // Verify: OnCombatUpdate was called
    ASSERT_TRUE(warrior->GetAI()->WasOnCombatUpdateCalled());
}
```

### 4. Test All 13 Classes
Location: `tests/integration/AllClassesCombatTest.cpp`

**Test Matrix**:
- ✅ Warrior: Heroic Strike, Rend, Battle Shout
- ✅ Paladin: Judgement, Crusader Strike, seals
- ✅ Hunter: Steady Shot, Arcane Shot, pet commands
- ✅ Rogue: Sinister Strike, Eviscerate, combo points
- ✅ Priest: Smite, Shadow Word: Pain, Mind Blast
- ✅ Shaman: Lightning Bolt, Flame Shock, totems
- ✅ Mage: Fireball, Frostbolt, Arcane Missiles
- ✅ Warlock: Shadow Bolt, Corruption, pet commands
- ✅ Druid: Wrath, Moonfire, shapeshifting
- ✅ Death Knight: Icy Touch, Plague Strike, rune management
- ✅ Monk: Jab, Tiger Palm, chi management
- ✅ Demon Hunter: Demon's Bite, Chaos Strike
- ✅ Evoker: Living Flame, Azure Strike

### 5. Test Gathering Interruption
Location: `tests/integration/GatheringCombatTest.cpp`

**Test Scenario**:
```cpp
TEST(GatheringCombat, BotFightsBackWhenInterrupted)
{
    // Setup: Bot is gathering herbs
    Bot* druid = CreateTestBot(CLASS_DRUID, 30);
    GameObject* herb = SpawnHerb(druid->GetPosition());
    druid->StartGathering(herb);

    // Action: Mob attacks while gathering
    Creature* mob = SpawnHostileMob(druid->GetPosition());
    mob->Attack(druid, true);

    // Wait for bot to react
    WaitForCombat(druid, 5000);
    ASSERT_TRUE(druid->IsInCombat());

    // Verify: Bot STOPS gathering and fights
    ASSERT_FALSE(druid->IsGathering());

    // Verify: Bot casts combat spells
    uint32 wrathCount = CountSpellCasts(druid, 5176); // Wrath
    ASSERT_GT(wrathCount, 0) << "Druid should cast Wrath when attacked";

    // Verify: Bot kills mob
    WaitForCombatEnd(druid, 30000);
    ASSERT_TRUE(mob->isDead());
}
```

### 6. Test Group Combat (Regression)
Location: `tests/integration/GroupCombatRegressionTest.cpp`

**Verify group combat still works**:
```cpp
TEST(GroupCombat, GroupBotsStillCastSpells)
{
    // Setup: Create 5-bot dungeon group
    Group* group = CreateTestGroup();
    Bot* tank = AddBotToGroup(group, CLASS_WARRIOR, ROLE_TANK);
    Bot* healer = AddBotToGroup(group, CLASS_PRIEST, ROLE_HEALER);
    Bot* dps1 = AddBotToGroup(group, CLASS_MAGE, ROLE_DPS);
    Bot* dps2 = AddBotToGroup(group, CLASS_ROGUE, ROLE_DPS);
    Bot* dps3 = AddBotToGroup(group, CLASS_HUNTER, ROLE_DPS);

    // Action: Pull dungeon boss
    Creature* boss = SpawnDungeonBoss();
    tank->Attack(boss, true);

    WaitForGroupCombat(group, 5000);

    // Verify: ALL bots cast spells
    ASSERT_GT(CountSpellCasts(tank, 78), 0);      // Heroic Strike
    ASSERT_GT(CountSpellCasts(healer, 2061), 0);  // Flash Heal
    ASSERT_GT(CountSpellCasts(dps1, 133), 0);     // Fireball
    ASSERT_GT(CountSpellCasts(dps2, 1752), 0);    // Sinister Strike
    ASSERT_GT(CountSpellCasts(dps3, 3044), 0);    // Arcane Shot

    // Verify: Boss dies
    WaitForCombatEnd(group, 60000);
    ASSERT_TRUE(boss->isDead());
}
```

### 7. Documentation
Location: `docs/COMBAT_ACTIVATION_GUIDE.md`

**Content**:
- Explanation of universal ClassAI activation
- Combat flow diagram (UpdateAI → OnCombatUpdate → ClassAI)
- Difference between group combat strategy and universal combat
- Troubleshooting guide for "bot doesn't cast spells"
- Performance considerations

---

## Implementation Steps

### Step 1: Remove Group Restriction (1 day)

**File**: `src/modules/Playerbot/AI/BotAI.cpp`

**Change 1**: Modify UpdateAI()
```cpp
// OLD:
if (IsInCombat() && HasActiveStrategy("group_combat"))
    OnCombatUpdate(diff);

// NEW:
if (IsInCombat())
    OnCombatUpdate(diff);
```

**Change 2**: Add debug logging (temporary)
```cpp
if (IsInCombat())
{
    static uint32 debugCounter = 0;
    if (++debugCounter % 100 == 0)
    {
        TC_LOG_INFO("module.playerbot", "🎯 OnCombatUpdate() called for bot {} (counter={})",
                    _bot->GetName(), debugCounter);
    }
    OnCombatUpdate(diff);
}
```

**Compile and verify**: No compilation errors

### Step 2: Verify ClassAI Initialization (1 day)

**File**: `src/modules/Playerbot/AI/BotAI.cpp`

**Add initialization check**:
```cpp
void BotAI::Initialize()
{
    // ... existing code ...

    _classAI = BotAIFactory::CreateClassAI(_bot, this);
    if (!_classAI)
    {
        TC_LOG_ERROR("module.playerbot", "❌ CRITICAL: Failed to create ClassAI for bot {} (class: {})",
                     _bot->GetName(), _bot->getClass());
        return;
    }

    TC_LOG_INFO("module.playerbot", "✅ ClassAI created for bot {} (class: {})",
                _bot->GetName(), _bot->getClass());
}
```

**Test**: Spawn 13 bots (one of each class), verify all get ClassAI

### Step 3: Test Solo Quest Combat (2 days)

**Test with 3 classes** (Warrior, Mage, Priest):
1. Create test bot (no group)
2. Attack training dummy
3. Monitor combat log
4. Verify spell casts (not just auto-attacks)
5. Verify OnCombatUpdate() called

**Expected Results**:
- Warrior: Heroic Strike every 3-5 seconds
- Mage: Fireball spam
- Priest: Smite/Mind Blast rotation

**If fails**: Add detailed debug logging in OnCombatUpdate()

### Step 4: Test All 13 Classes (2 days)

**Test Matrix** (automate with script):
```bash
for class in WARRIOR PALADIN HUNTER ROGUE PRIEST SHAMAN MAGE WARLOCK DRUID DK MONK DH EVOKER; do
    echo "Testing $class..."
    spawn_bot $class 30
    attack_dummy
    wait_for_combat
    verify_spell_casts
    cleanup
done
```

**Success Criteria**: All 13 classes cast at least 1 class-appropriate spell

### Step 5: Test Gathering Interruption (1 day)

**Test Scenario**:
1. Bot starts gathering herb/ore
2. Mob spawns and attacks
3. Verify bot stops gathering
4. Verify bot enters combat and casts spells
5. Verify bot resumes gathering after combat

**Classes to Test**: Druid, Rogue (stealth), Warrior

### Step 6: Regression Testing (1 day)

**Verify group combat still works**:
1. Create 5-bot group
2. Enter dungeon
3. Pull boss
4. Verify ALL bots cast spells
5. Verify tank holds aggro
6. Verify healer heals
7. Verify DPS deal damage
8. Verify boss dies

**If regression detected**: Investigate group combat strategy interaction

### Step 7: Documentation & Cleanup (1 day)

1. Write combat activation guide
2. Remove temporary debug logging
3. Update ARCHITECTURE.md with new flow
4. Create troubleshooting guide
5. Final code review

---

## Success Criteria

### Functional Requirements
- ✅ OnCombatUpdate() called for ALL bots in combat (not just groups)
- ✅ Solo questing bots cast class-appropriate spells
- ✅ Gathering bots fight back when attacked
- ✅ Group combat still works (no regression)
- ✅ All 13 classes verified working
- ✅ Dungeon/raid combat works

### Combat Effectiveness
- ✅ Solo bot kills level-appropriate mob in <30 seconds
- ✅ Bot uses 80%+ of available GCDs (Global Cooldowns)
- ✅ Bot manages resources correctly (rage/mana/energy/runes)
- ✅ Bot uses cooldowns appropriately
- ✅ Bot doesn't stand idle during combat

### Performance
- ✅ OnCombatUpdate() <1ms per bot per frame
- ✅ ClassAI::Update() <0.5ms per bot per frame
- ✅ No performance regression with 100+ bots in combat
- ✅ No memory leaks after 1000 combat cycles

### Code Quality
- ✅ Clean code without workarounds
- ✅ No group-specific restrictions in core combat logic
- ✅ Proper separation: Strategies coordinate, ClassAI executes
- ✅ Full documentation of combat flow
- ✅ Comprehensive test coverage

---

## Dependencies

### Requires
- Phase 2.1 complete (BehaviorManager base class - for context)
- Phase 2.2 complete (CombatMovementStrategy - for positioning)
- Existing ClassAI implementations (all 13 classes)
- TrinityCore combat APIs

### Blocks
- Phase 2.5 (IdleStrategy needs working combat to delegate)
- Phase 2.6 (Integration testing requires combat working)
- Full bot functionality (combat is core feature)

---

## Risk Mitigation

### Risk: Performance regression with universal combat
**Mitigation**:
- Profile OnCombatUpdate() before/after change
- Ensure ClassAI implementations are optimized
- Test with 500+ bots in combat

### Risk: Breaking group combat coordination
**Mitigation**:
- Keep GroupCombatStrategy separate (for coordination)
- GroupCombatStrategy observes, ClassAI executes
- Comprehensive regression testing

### Risk: Combat spam in logs
**Mitigation**:
- Use throttled debug logging (every 100th call)
- Remove debug logging after testing phase
- Configure log levels properly

### Risk: Bots stuck in combat state
**Mitigation**:
- Verify IsInCombat() state management
- Test combat → idle transitions
- Add automatic combat timeout (5 minutes)

---

## Testing Checklist

### Solo Combat Testing
- [ ] Warrior solo quest combat
- [ ] Mage solo quest combat
- [ ] Priest solo quest combat
- [ ] Hunter solo quest combat (with pet)
- [ ] Rogue solo quest combat (stealth opener)
- [ ] All other 8 classes solo combat

### Interruption Testing
- [ ] Gathering herbs, attacked by mob
- [ ] Fishing, attacked by mob
- [ ] Skinning, attacked by mob
- [ ] Trading with NPC, attacked by mob

### Group Combat Testing (Regression)
- [ ] 5-bot dungeon group combat
- [ ] 10-bot raid combat
- [ ] 25-bot raid combat
- [ ] Tank holds aggro with threat skills
- [ ] Healer keeps group alive
- [ ] DPS use full rotations

### Edge Cases
- [ ] Bot level 1 combat (minimal spells)
- [ ] Bot level 80 combat (full rotation)
- [ ] Dual-wielding classes
- [ ] Pet classes (Hunter, Warlock, DK)
- [ ] Shapeshifting (Druid)
- [ ] Stealth classes (Rogue, Druid)

---

## Next Phase

After completion, proceed to **Phase 2.4: Refactor Managers (Remove Automation Singletons)**

---

**Last Updated**: 2025-01-13
**Next Review**: 2025-02-03
