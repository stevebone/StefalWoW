# Zenflow Task: Healing & Tank/Aggro System Analysis

**Priority**: P0 (Critical - Core Combat Functionality)  
**Estimated Duration**: 4-6 hours  
**Prerequisites**: None (standalone analysis)

---

## Context

Users report that:
1. **Healers don't heal group members** - HealingTargetSelector seems broken
2. **Tanks don't hold aggro properly** - Aggro management issues
3. **Group combat doesn't trigger correctly** - GroupCombatTrigger problems

This analysis will identify root causes and create a fix plan.

---

## Phase 1: Healing System Analysis (2h)

### 1.1 Core Healing Components

Find and analyze these files:
```
src/modules/Playerbot/AI/
├── Combat/HealingTargetSelector.h/.cpp
├── Strategy/Healer*.cpp
├── Triggers/HealerTriggers.cpp
└── Actions/HealerActions.cpp
```

**Questions to answer:**
- How does HealingTargetSelector find targets?
- Does it check group members correctly?
- What's the target priority logic?
- Are there range/visibility checks that might fail?

### 1.2 Class-Specific Healing

Analyze healer class implementations:
```
src/modules/Playerbot/AI/Strategy/Classes/
├── Priest/HolyPriestStrategy.cpp
├── Druid/RestorationDruidStrategy.cpp
├── Paladin/HolyPaladinStrategy.cpp
├── Shaman/RestorationShamanStrategy.cpp
└── (any other healer specs)
```

**Questions to answer:**
- How do class strategies select heal targets?
- Do they use HealingTargetSelector or own logic?
- Are there inconsistencies between classes?

### 1.3 Group Member Detection

Find how healers detect group members:
```
Search for:
- GetGroup()
- GetGroupMembers()
- GroupReference
- PartyMember
- RaidMember
```

**Questions to answer:**
- Is group membership detected correctly?
- Are bots in the same group as the player?
- Does cross-group healing work (raids)?

---

## Phase 2: Tank/Aggro System Analysis (2h)

### 2.1 Core Threat Components

Find and analyze:
```
src/modules/Playerbot/AI/
├── Combat/ThreatCoordinator.h/.cpp
├── Combat/BotThreatManager.h/.cpp
├── Strategy/Tank*.cpp
└── Triggers/TankTriggers.cpp
```

**Questions to answer:**
- How is threat calculated?
- How do tanks know when to taunt?
- Is there a threat threshold for action?

### 2.2 Tank Class Implementations

Analyze tank class strategies:
```
src/modules/Playerbot/AI/Strategy/Classes/
├── Warrior/ProtectionWarriorStrategy.cpp
├── Paladin/ProtectionPaladinStrategy.cpp
├── Druid/GuardianDruidStrategy.cpp
├── DeathKnight/BloodDeathKnightStrategy.cpp
└── (any other tank specs)
```

**Questions to answer:**
- How do tanks establish initial aggro?
- Is there a pull rotation?
- How is multi-target threat handled (AoE tanking)?

### 2.3 Taunt Logic

Find taunt-related code:
```
Search for:
- Taunt
- TauntAction
- AggroAction
- ThreatAction
- Provoke
```

**Questions to answer:**
- When do tanks taunt?
- Is there taunt coordination (avoid wasting taunts)?
- Are tank swaps handled?

---

## Phase 3: Group Combat Integration (1h)

### 3.1 Combat Triggers

Find and analyze:
```
src/modules/Playerbot/AI/
├── Triggers/GroupCombatTrigger.cpp
├── Triggers/CombatTriggers.cpp
└── Strategy/GroupCombatStrategy.cpp
```

**Questions to answer:**
- What triggers "group is in combat"?
- Does it check all group members?
- Are there timing issues (detection delay)?

### 3.2 Role Coordination

Find how roles coordinate:
```
Search for:
- RoleCoordinator
- GroupCoordinator
- HealerCoordinator
- TankCoordinator
```

**Questions to answer:**
- How do healers know who the tank is?
- How do tanks know who to protect?
- Is there a target assignment system?

---

## Phase 4: Create Fix Recommendations

### 4.1 Document Findings

Create summary of:
1. **Root causes** - Why healing/tanking fails
2. **Code locations** - Exact files and line numbers
3. **Dependencies** - What systems are involved

### 4.2 Prioritize Fixes

Rank issues by:
- **Impact**: How many users affected
- **Severity**: How broken is the feature
- **Effort**: How hard to fix

### 4.3 Create Implementation Plan

For each fix:
- Describe the change
- List files to modify
- Estimate effort (hours)
- Note any risks

---

## Deliverables

1. **HEALING_ANALYSIS.md** - Complete healing system analysis
2. **TANK_AGGRO_ANALYSIS.md** - Complete tank/aggro analysis
3. **GROUP_COMBAT_ANALYSIS.md** - Group combat trigger analysis
4. **FIX_RECOMMENDATIONS.md** - Prioritized fix list with implementation plan

---

## Search Commands

```bash
# Find healing-related files
find . -name "*.cpp" -o -name "*.h" | xargs grep -l "Heal" | head -50

# Find threat-related files
find . -name "*.cpp" -o -name "*.h" | xargs grep -l "Threat\|Aggro\|Taunt" | head -50

# Find group combat triggers
grep -rn "GroupCombat\|InCombat" --include="*.cpp" --include="*.h"

# Find target selection
grep -rn "SelectTarget\|FindTarget\|GetTarget" --include="*.cpp" --include="*.h"
```

---

## Success Criteria

- [ ] HealingTargetSelector logic fully understood
- [ ] Root cause of "healers don't heal group" identified
- [ ] ThreatCoordinator/BotThreatManager relationship mapped
- [ ] Root cause of "tanks don't hold aggro" identified
- [ ] GroupCombatTrigger logic analyzed
- [ ] Fix recommendations created with effort estimates
- [ ] All findings documented in markdown files
