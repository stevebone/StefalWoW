# ClassAI Refactoring Status & Phase 5 Integration Plan

## Executive Summary

This document provides a complete analysis of ClassAI refactoring status and a strategic plan for integrating Phase 5 decision systems (ActionPriorityQueue, BehaviorTree, ActionScoringEngine).

**Current Situation**:
- ✅ **37 specs** have refactored header-based implementations
- ❌ **2 specs** (Evoker) still need refactoring
- ✅ **1 spec** (Arms Warrior) has Phase 5 integration complete
- ⏳ **36 specs** ready for Phase 5 integration (refactored but not integrated)

---

## Table of Contents

1. [Refactoring Status by Class](#refactoring-status-by-class)
2. [Phase 5 Integration Status](#phase-5-integration-status)
3. [Integration Priority Plan](#integration-priority-plan)
4. [Recommended Workflow](#recommended-workflow)
5. [Timeline & Resources](#timeline--resources)

---

## Refactoring Status by Class

### ✅ Fully Refactored Classes (Header-Based)

| Class | Specs Refactored | Files | Status |
|-------|------------------|-------|--------|
| **Warrior** | 3/3 | Arms, Fury, Protection | ✅ Complete |
| **Paladin** | 3/3 | Holy, Protection, Retribution | ✅ Complete |
| **Hunter** | 3/3 | Beast Mastery, Marksmanship, Survival | ✅ Complete |
| **Rogue** | 3/3 | Assassination, Outlaw, Subtlety | ✅ Complete |
| **Priest** | 3/3 | Discipline, Holy, Shadow | ✅ Complete |
| **Death Knight** | 3/3 | Blood, Frost, Unholy | ✅ Complete |
| **Shaman** | 3/3 | Elemental, Enhancement, Restoration | ✅ Complete |
| **Mage** | 3/3 | Arcane, Fire, Frost | ✅ Complete |
| **Warlock** | 3/3 | Affliction, Demonology, Destruction | ✅ Complete |
| **Monk** | 3/3 | Brewmaster, Mistweaver, Windwalker | ✅ Complete |
| **Druid** | 4/4 | Balance, Feral, Guardian, Restoration | ✅ Complete |
| **Demon Hunter** | 2/2 | Havoc, Vengeance | ✅ Complete |

**Total Refactored**: 37 specs across 12 classes

### ❌ Not Yet Refactored

| Class | Specs Missing | Files Needed | Priority |
|-------|---------------|--------------|----------|
| **Evoker** | 2/2 | Devastation, Preservation | Medium |

**Total Pending Refactoring**: 2 specs

---

## Phase 5 Integration Status

### ✅ Integrated Specs (Phase 5 Complete)

| Class | Spec | ActionPriorityQueue | BehaviorTree | Status |
|-------|------|---------------------|--------------|--------|
| Warrior | Arms | ✅ 11 spells | ✅ 3-tier tree | 🎯 Reference Implementation |

**Total Integrated**: 1 spec

### ⏳ Ready for Integration (Refactored, Not Integrated)

| Class | Specs | Count | Priority |
|-------|-------|-------|----------|
| Warrior | Fury, Protection | 2 | 🔴 High |
| Paladin | Holy, Protection, Retribution | 3 | 🔴 High |
| Priest | Discipline, Holy, Shadow | 3 | 🔴 High |
| Hunter | BM, MM, Survival | 3 | 🟡 Medium |
| Rogue | Assassination, Outlaw, Subtlety | 3 | 🟡 Medium |
| Mage | Arcane, Fire, Frost | 3 | 🟡 Medium |
| Warlock | Affliction, Demonology, Destruction | 3 | 🟡 Medium |
| Shaman | Elemental, Enhancement, Restoration | 3 | 🟡 Medium |
| Death Knight | Blood, Frost, Unholy | 3 | 🟢 Low |
| Monk | Brewmaster, Mistweaver, Windwalker | 3 | 🟢 Low |
| Druid | Balance, Feral, Guardian, Restoration | 4 | 🟡 Medium |
| Demon Hunter | Havoc, Vengeance | 2 | 🟢 Low |

**Total Ready**: 36 specs

### ❌ Requires Refactoring First

| Class | Specs | Action |
|-------|-------|--------|
| Evoker | Devastation, Preservation | Refactor → Then Integrate |

**Total Pending Refactoring**: 2 specs

---

## Integration Priority Plan

### Phase 1: High Priority - Core Classes (9 specs, ~2-3 weeks)

**Objective**: Cover most popular/common classes first

| Priority | Class | Spec | Rationale |
|----------|-------|------|-----------|
| 1 | Warrior | Fury | Complete Warrior class (Arms done) |
| 2 | Warrior | Protection | Tank representation needed |
| 3 | Paladin | Holy | Popular healer class |
| 4 | Paladin | Protection | Tank representation |
| 5 | Paladin | Retribution | Popular DPS spec |
| 6 | Priest | Discipline | Unique healer playstyle |
| 7 | Priest | Holy | Popular healer |
| 8 | Priest | Shadow | Popular caster DPS |
| 9 | Hunter | Beast Mastery | Most popular Hunter spec |

**Estimated Time**: 2-3 hours per spec (research rotations + implement) = 18-27 hours total

### Phase 2: Medium Priority - Popular DPS (15 specs, ~4-5 weeks)

| Class | Specs | Count |
|-------|-------|-------|
| Hunter | Marksmanship, Survival | 2 |
| Rogue | Assassination, Outlaw, Subtlety | 3 |
| Mage | Arcane, Fire, Frost | 3 |
| Warlock | Affliction, Demonology, Destruction | 3 |
| Druid | Balance, Feral, Guardian, Restoration | 4 |

**Estimated Time**: 2 hours per spec = 30 hours total

### Phase 3: Medium Priority - Support Classes (6 specs, ~2 weeks)

| Class | Specs | Count |
|-------|-------|-------|
| Shaman | Elemental, Enhancement, Restoration | 3 |
| Death Knight | Blood, Frost, Unholy | 3 |

**Estimated Time**: 2 hours per spec = 12 hours total

### Phase 4: Low Priority - Newer Classes (5 specs, ~1-2 weeks)

| Class | Specs | Count |
|-------|-------|-------|
| Monk | Brewmaster, Mistweaver, Windwalker | 3 |
| Demon Hunter | Havoc, Vengeance | 2 |

**Estimated Time**: 2 hours per spec = 10 hours total

### Phase 5: Refactor + Integrate - Evoker (2 specs, ~2 weeks)

**Special Case**: Evoker requires refactoring first

**Steps**:
1. Create `DevastationEvokerRefactored.h` (8 hours)
2. Create `PreservationEvokerRefactored.h` (8 hours)
3. Integrate Phase 5 systems for both (4 hours)

**Estimated Time**: 20 hours total

---

## Recommended Workflow

### For Refactored Classes (36 specs)

**Pattern** (2 hours per spec):

```cpp
// 1. Add Phase 5 includes (5 minutes)
#include "../../Decision/ActionPriorityQueue.h"
#include "../../Decision/BehaviorTree.h"
#include "../../BotAI.h"

// 2. Update InitializeXxxRotation() method (1.5 hours)
void InitializeXxxRotation()
{
    using namespace bot::ai;
    using namespace bot::ai::BehaviorTreeBuilder;

    BotAI* ai = this->GetBot()->GetBotAI();
    if (!ai) return;

    // Register spells in ActionPriorityQueue
    auto* queue = ai->GetActionPriorityQueue();
    if (queue)
    {
        // Research optimal rotation for spec
        // Register 10-15 spells with priorities/conditions
        // Reference: Icy Veins, WoWHead, SimulationCraft
    }

    // Build BehaviorTree
    auto* tree = ai->GetBehaviorTree();
    if (tree)
    {
        // Create hierarchical combat flow
        // Execute phase → Cooldowns → Rotation → Fillers
    }
}

// 3. Test in-game (30 minutes)
// - Spawn bot
// - Enable debug logging
// - Verify spell registration
// - Verify decision fusion votes
```

**Reference**: Arms Warrior implementation in `ArmsWarriorRefactored.h`

### For Evoker (Requires Refactoring)

**Step 1**: Refactor to Header-Based (8 hours per spec)

1. Create `DevastationEvokerRefactored.h`
2. Inherit from `RangedDpsSpecialization<ManaResource>`
3. Move rotation logic from `EvokerAI.cpp`
4. Eliminate duplicate code using templates

**Step 2**: Integrate Phase 5 (2 hours per spec)

Follow same pattern as refactored classes above.

---

## Detailed Integration Checklist

### Per-Spec Checklist

- [ ] **Research Rotation** (30 min)
  - [ ] Review Icy Veins guide
  - [ ] Identify priority abilities
  - [ ] Note cooldown usage
  - [ ] Identify procs/conditions
  - [ ] Document execute phase (<20% HP)

- [ ] **ActionPriorityQueue** (45 min)
  - [ ] Add Phase 5 includes
  - [ ] Get BotAI pointer
  - [ ] Register EMERGENCY spells (execute abilities)
  - [ ] Register CRITICAL spells (major cooldowns)
  - [ ] Register HIGH spells (core rotation)
  - [ ] Register MEDIUM spells (situational)
  - [ ] Register LOW spells (fillers)
  - [ ] Add conditions for procs
  - [ ] Add conditions for AoE detection
  - [ ] Add conditions for resource thresholds

- [ ] **BehaviorTree** (45 min)
  - [ ] Design tree structure on paper
  - [ ] Create Execute Phase selector
  - [ ] Create Cooldown Usage sequence
  - [ ] Create Standard Rotation sequence
  - [ ] Add filler actions
  - [ ] Set tree root
  - [ ] Add debug logging

- [ ] **Testing** (30 min)
  - [ ] Compile (fix any errors)
  - [ ] Spawn bot in-game
  - [ ] Enable debug logging
  - [ ] Verify log output shows registration
  - [ ] Attack target dummy
  - [ ] Verify decision fusion votes
  - [ ] Verify spell casting follows priority
  - [ ] Test execute phase transition
  - [ ] Test cooldown usage on boss

---

## Resource Requirements

### Time Estimates

| Phase | Specs | Hours/Spec | Total Hours | Calendar Time |
|-------|-------|------------|-------------|---------------|
| Phase 1 (High) | 9 | 2 | 18 | 2-3 weeks |
| Phase 2 (Medium DPS) | 15 | 2 | 30 | 4-5 weeks |
| Phase 3 (Support) | 6 | 2 | 12 | 2 weeks |
| Phase 4 (Low) | 5 | 2 | 10 | 1-2 weeks |
| Phase 5 (Evoker) | 2 | 10 | 20 | 2 weeks |
| **Total** | **37** | - | **90** | **11-14 weeks** |

### Parallel Execution

If multiple developers work in parallel:
- **2 developers**: 6-7 weeks
- **3 developers**: 4-5 weeks
- **4 developers**: 3-4 weeks

### Automation Opportunities

**Consider Creating**:
1. **Spell Registration Script**: Parse SimulationCraft APLs to auto-generate spell registrations
2. **Tree Template Generator**: Create basic tree structure from role (tank/healer/DPS)
3. **Testing Harness**: Automated bot spawning + log verification

---

## Integration Examples

### Tank Example (Protection Paladin)

```cpp
// Emergency (EMERGENCY priority)
queue->RegisterSpell(SPELL_GUARDIAN_OF_ANCIENT_KINGS, SpellPriority::EMERGENCY, SpellCategory::DEFENSIVE);
queue->AddCondition(SPELL_GUARDIAN_OF_ANCIENT_KINGS,
    [](Player* bot, Unit*) { return bot->GetHealthPct() < 20.0f; },
    "Emergency: HP < 20%");

// Critical (CRITICAL priority)
queue->RegisterSpell(SPELL_ARDENT_DEFENDER, SpellPriority::CRITICAL, SpellCategory::DEFENSIVE);
queue->RegisterSpell(SPELL_AVENGERS_SHIELD, SpellPriority::CRITICAL, SpellCategory::OFFENSIVE);

// High (HIGH priority)
queue->RegisterSpell(SPELL_SHIELD_OF_THE_RIGHTEOUS, SpellPriority::HIGH, SpellCategory::DEFENSIVE);
queue->RegisterSpell(SPELL_JUDGMENT, SpellPriority::HIGH, SpellCategory::OFFENSIVE);

// Behavior Tree
auto root = Selector("Prot Paladin", {
    // Emergency survival
    Sequence("Emergency", {
        Condition("HP < 20%", [](Player* bot, Unit*) {
            return bot->GetHealthPct() < 20.0f;
        }),
        Action("Cast Guardian", [this](Player* bot, Unit*) {
            // Cast emergency defensive
        })
    }),
    // Maintain active mitigation
    Selector("Active Mitigation", {
        Condition("Shield active", [](Player* bot, Unit*) {
            return bot->HasAura(SPELL_SHIELD_OF_THE_RIGHTEOUS);
        }),
        Action("Cast Shield", [this](Player* bot, Unit* target) {
            // Cast Shield of the Righteous
        })
    }),
    // Threat generation
    // ...
});
```

### Healer Example (Holy Priest)

```cpp
// Emergency (EMERGENCY priority)
queue->RegisterSpell(SPELL_GUARDIAN_SPIRIT, SpellPriority::EMERGENCY, SpellCategory::HEALING);
queue->AddCondition(SPELL_GUARDIAN_SPIRIT,
    [](Player* bot, Unit*) {
        // Cast on tank when very low HP
        if (Group* group = bot->GetGroup())
        {
            for (auto ref : *group)
            {
                if (Player* member = ref->GetSource())
                {
                    if (member->GetHealthPct() < 15.0f && IsTank(member))
                        return true;
                }
            }
        }
        return false;
    },
    "Tank HP < 15%");

// Critical (CRITICAL priority)
queue->RegisterSpell(SPELL_PRAYER_OF_MENDING, SpellPriority::CRITICAL, SpellCategory::HEALING);
queue->RegisterSpell(SPELL_RENEW, SpellPriority::HIGH, SpellCategory::HEALING);

// Behavior Tree
auto root = Selector("Holy Priest", {
    // Emergency tank save
    Sequence("Tank Emergency", {
        Condition("Tank dying", [](Player* bot, Unit*) {
            // Check tank HP
        }),
        Action("Guardian Spirit", [this](Player* bot, Unit* tank) {
            // Cast on tank
        })
    }),
    // Group healing
    Sequence("Group Healing", {
        Condition("Group damage", [](Player* bot, Unit*) {
            // Check group HP average
        }),
        Action("Prayer of Healing", [this](Player* bot, Unit*) {
            // Cast AoE heal
        })
    }),
    // Single target healing
    // ...
});
```

### DPS Example (Fire Mage)

```cpp
// Critical (CRITICAL priority) - Big cooldowns
queue->RegisterSpell(SPELL_COMBUSTION, SpellPriority::CRITICAL, SpellCategory::OFFENSIVE);
queue->RegisterSpell(SPELL_PYROBLAST, SpellPriority::CRITICAL, SpellCategory::DAMAGE_SINGLE);
queue->AddCondition(SPELL_PYROBLAST,
    [](Player* bot, Unit*) {
        return bot->HasAura(SPELL_HOT_STREAK); // Only with proc
    },
    "Hot Streak proc");

// High (HIGH priority)
queue->RegisterSpell(SPELL_FIREBALL, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);
queue->RegisterSpell(SPELL_FIRE_BLAST, SpellPriority::HIGH, SpellCategory::DAMAGE_SINGLE);

// Behavior Tree
auto root = Selector("Fire Mage", {
    // Combustion window
    Sequence("Combustion Phase", {
        Condition("Combustion active", [](Player* bot, Unit*) {
            return bot->HasAura(SPELL_COMBUSTION);
        }),
        Selector("Burn Phase", {
            Action("Pyroblast (Hot Streak)", [this](Player* bot, Unit* target) {
                // Cast Pyroblast with proc
            }),
            Action("Fire Blast", [this](Player* bot, Unit* target) {
                // Generate Hot Streak
            })
        })
    }),
    // Standard rotation
    Sequence("Standard", {
        Action("Fire Blast (heating up)", [this](Player* bot, Unit* target) {
            // Convert Heating Up to Hot Streak
        }),
        Action("Fireball", [this](Player* bot, Unit* target) {
            // Filler
        })
    })
});
```

---

## Testing Strategy

### Unit Testing

✅ **Already Complete** (Phase 6):
- ActionPriorityQueue_tests.cpp
- BehaviorTree_tests.cpp
- DecisionFusionSystem_tests.cpp

### Integration Testing (Per Spec)

**In-Game Test Protocol**:

```bash
# 1. Compile
cd build && cmake .. && make -j$(nproc)

# 2. Launch server
./worldserver

# 3. In-game commands
.playerbot add TestWarrior  # Or appropriate class
.playerbot debug fusion 1
.playerbot debug priority 1
.playerbot debug behavior 1

# 4. Verify logs
# Should see:
# ⚔️  <SPEC>: Registered X spells in ActionPriorityQueue
# 🌲 <SPEC>: BehaviorTree initialized with hierarchical combat flow
# 🎯 DECISION FUSION: Recommended action <ID> - 3 votes

# 5. Combat test
.go xyz 123 456 789  # Go to area with mobs
# Attack target, verify:
# - Correct spell priorities
# - Execute phase transition at 20% HP
# - Cooldown usage on bosses
# - AoE detection works
# - Proc-based abilities trigger correctly

# 6. Performance test
.playerbot add 10  # Spawn 10 bots
# Monitor:
# - CPU usage (<10% increase)
# - Memory usage (<15MB increase)
# - No crashes/errors in logs
```

---

## Success Metrics

### Per Spec

- ✅ Compiles without errors
- ✅ 10-15 spells registered in ActionPriorityQueue
- ✅ BehaviorTree has 3+ tiers (execute, cooldowns, rotation)
- ✅ All conditions have descriptive reasoning
- ✅ Debug logs show 3 votes from DecisionFusion
- ✅ Bot casts appropriate spells in combat
- ✅ Execute phase activates at <20% HP
- ✅ Cooldowns used on bosses
- ✅ AoE abilities trigger with 3+ targets

### Overall Project

- ✅ All 39 specs integrated
- ✅ Zero compilation errors
- ✅ Zero runtime crashes
- ✅ <10% performance overhead
- ✅ Comprehensive testing completed

---

## Next Immediate Actions

### Today/This Week:

1. ✅ **Arms Warrior** - Already complete (reference)
2. **Fury Warrior** - Similar to Arms (2 hours)
3. **Protection Warrior** - Tank implementation (2 hours)
4. **Test all 3 Warriors** together (1 hour)
5. **Document Warrior integration** (1 hour)

**Total**: 6 hours to complete entire Warrior class

### Next Week:

6. **Paladin - All 3 specs** (6 hours)
7. **Priest - All 3 specs** (6 hours)
8. **Test Phase 1 specs** (2 hours)

**Total**: 14 hours for Phase 1 completion

---

## Conclusion

**Summary**:
- 37 specs ready for immediate Phase 5 integration (already refactored)
- 2 specs need refactoring first (Evoker)
- 1 spec complete (Arms Warrior - reference implementation)
- Clear 5-phase plan with priorities
- Estimated 11-14 weeks for full completion (solo)
- 3-4 weeks with 4 developers in parallel

**Recommendation**: Start with **Phase 1** (Warrior, Paladin, Priest, Hunter BM) to establish pattern and momentum. These 9 specs represent the most common classes and will provide maximum player impact.

**Reference Materials**:
- Integration guide: `docs/PHASE5_CLASSAI_INTEGRATION_GUIDE.md`
- Reference implementation: `ArmsWarriorRefactored.h`
- Unit tests: `tests/Phase5/`

**Let's do this! 🚀**
