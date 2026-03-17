# Zenflow Task: AI/Behavior System Analysis

**Priority**: P1 (High - Core Bot Intelligence)  
**Estimated Duration**: 6-8 hours  
**Prerequisites**: Combat Refactoring complete, Event System available

---

## Context

The AI/Behavior system controls what bots DO and WHEN they do it. This is the "brain" of the bot - it decides:
- Which strategy to use (Combat, Grinding, Questing, etc.)
- What actions to take (Attack, Heal, Move, etc.)
- When to switch behaviors (Combat → Loot → Follow)
- How to prioritize competing goals

This system should integrate with the new Event-Driven architecture from Combat Refactoring Phase 3.

---

## Phase 1: Strategy System Analysis (2h)

### 1.1 Strategy Architecture

Find and analyze the strategy system:
```bash
find src/modules/Playerbot/AI/Strategy -name "*.h" -o -name "*.cpp" | head -50
```

**Key files to examine:**
```
src/modules/Playerbot/AI/Strategy/
├── Strategy.h/.cpp              <- Base strategy class
├── StrategyManager.h/.cpp       <- Strategy selection/switching
├── Multiplier.h/.cpp            <- Priority multipliers
├── ActionSelector.h/.cpp        <- Action selection logic
└── [Various Strategy Types]/    <- Combat, Grinding, Quest, etc.
```

**Questions to answer:**
- How are strategies selected?
- How do strategies switch (Combat → NonCombat)?
- Is there a state machine?
- What's the strategy inheritance hierarchy?

### 1.2 Strategy Types Inventory

List all strategy types:
```bash
grep -rn "class.*Strategy" src/modules/Playerbot/AI/Strategy --include="*.h" | head -100
```

Create inventory:
| Strategy | Purpose | Used By | Quality |
|----------|---------|---------|---------|
| CombatStrategy | Combat behavior | All | ? |
| GrindStrategy | Mob grinding | Solo | ? |
| QuestStrategy | Quest completion | Solo | ? |
| FollowStrategy | Follow master | Group | ? |
| ... | ... | ... | ... |

### 1.3 Strategy Selection Logic

Find how strategies are chosen:
```bash
grep -rn "SelectStrategy\|AddStrategy\|RemoveStrategy\|SetStrategy" src/modules/Playerbot --include="*.cpp"
```

**Questions to answer:**
- What triggers strategy changes?
- Is there priority between strategies?
- Can multiple strategies be active?
- How are conflicts resolved?

---

## Phase 2: Action/Trigger System Analysis (2h)

### 2.1 Action System

Find and analyze actions:
```bash
find src/modules/Playerbot/AI/Actions -name "*.h" -o -name "*.cpp" | head -50
```

**Key files:**
```
src/modules/Playerbot/AI/Actions/
├── Action.h/.cpp                <- Base action class
├── ActionContext.h              <- Action execution context
├── CombatActions.h/.cpp         <- Combat-specific actions
├── MovementActions.h/.cpp       <- Movement actions
├── QuestActions.h/.cpp          <- Quest actions
└── ...
```

**Questions to answer:**
- How are actions defined?
- How are actions executed?
- What's the action lifecycle?
- Are actions reusable across strategies?

### 2.2 Trigger System

Find and analyze triggers:
```bash
find src/modules/Playerbot/AI/Triggers -name "*.h" -o -name "*.cpp" | head -50
```

**Key files:**
```
src/modules/Playerbot/AI/Triggers/
├── Trigger.h/.cpp               <- Base trigger class
├── CombatTriggers.h/.cpp        <- Combat triggers
├── HealthTriggers.h/.cpp        <- Health-based triggers
├── GenericTriggers.h/.cpp       <- Generic triggers
└── ...
```

**Questions to answer:**
- How do triggers work?
- What's the trigger evaluation frequency?
- Are triggers event-driven or polling-based?
- How do triggers connect to actions?

### 2.3 Action-Trigger Binding

Find how actions and triggers connect:
```bash
grep -rn "BindTrigger\|RegisterAction\|TriggerAction" src/modules/Playerbot --include="*.cpp"
```

**Questions to answer:**
- How are triggers bound to actions?
- Is there a priority system?
- Can one trigger invoke multiple actions?
- How are action conflicts resolved?

---

## Phase 3: Decision Making Analysis (1.5h)

### 3.1 Priority System

Find priority/weight calculations:
```bash
grep -rn "Priority\|Weight\|Relevance\|Score" src/modules/Playerbot/AI --include="*.h" --include="*.cpp" | head -50
```

**Questions to answer:**
- How are action priorities calculated?
- Are there dynamic priorities?
- What factors influence priority?
- Is there a cooldown/throttle system?

### 3.2 State Machine (if exists)

Find state machine implementation:
```bash
grep -rn "State\|StateMachine\|Transition\|CurrentState" src/modules/Playerbot/AI --include="*.h" | head -30
```

**Questions to answer:**
- Is there a formal state machine?
- What states exist?
- How are transitions handled?
- Is the state machine hierarchical?

### 3.3 Decision Flow

Trace a complete decision:
```
Trigger fires (e.g., "enemy in range")
    ↓
??? (what happens here?)
    ↓
Action executed (e.g., "attack enemy")
```

Document the complete flow with code references.

---

## Phase 4: Performance Analysis (1h)

### 4.1 Update Frequency

Find how often AI updates:
```bash
grep -rn "UpdateAI\|Update.*diff\|AI.*Update" src/modules/Playerbot --include="*.cpp" | head -30
```

**Questions to answer:**
- How often does AI update? (every tick? throttled?)
- What's the performance cost per bot?
- Are there O(N²) or worse algorithms?

### 4.2 Trigger Evaluation

Analyze trigger evaluation cost:
```bash
grep -rn "IsActive\|Check\|Evaluate" src/modules/Playerbot/AI/Triggers --include="*.cpp" | head -30
```

**Questions to answer:**
- How many triggers are evaluated per update?
- Are inactive triggers skipped?
- Is there trigger caching?

### 4.3 Action Selection

Analyze action selection cost:
```bash
grep -rn "SelectAction\|GetAction\|BestAction" src/modules/Playerbot/AI --include="*.cpp"
```

**Questions to answer:**
- How is the best action selected?
- Is there sorting/iteration over all actions?
- Can this be optimized?

---

## Phase 5: Event System Integration (1h)

### 5.1 Current Event Usage

Check if AI uses events:
```bash
grep -rn "Event\|Subscribe\|Publish\|OnEvent" src/modules/Playerbot/AI --include="*.cpp" | head -30
```

### 5.2 Integration Opportunities

Based on Combat Refactoring's CombatEventRouter, identify:
- Which triggers could become event subscribers?
- Which actions could be event-triggered?
- How to reduce polling with events?

**Proposed Integration:**
```
CombatEventRouter
    ├── ENEMY_SPOTTED → CombatStrategy activation
    ├── HEALTH_LOW → HealAction trigger
    ├── COMBAT_END → LootStrategy activation
    └── ...
```

---

## Phase 6: Gap Analysis & Recommendations

### 6.1 Missing Behaviors

Identify missing behavior types:
- [ ] Dungeon-specific behaviors
- [ ] PvP-specific behaviors (Arena, BG)
- [ ] Profession behaviors
- [ ] Social behaviors (trade, chat)
- [ ] Emergency behaviors (stuck, death)

### 6.2 Code Quality Issues

Identify:
- Duplicate code between strategies
- Hardcoded values that should be config
- Dead/unused strategies
- Overly complex decision logic

### 6.3 Performance Issues

Identify:
- O(N²) algorithms
- Excessive polling
- Memory leaks
- Cache misses

---

## Deliverables

Create in `.claude/analysis/`:

1. **AI_BEHAVIOR_ARCHITECTURE.md**
   - Strategy system overview
   - Action/Trigger system overview
   - Decision flow diagrams
   - Component relationships

2. **AI_BEHAVIOR_INVENTORY.md**
   - All strategies with descriptions
   - All actions with descriptions
   - All triggers with descriptions
   - Quality assessment for each

3. **AI_BEHAVIOR_PERFORMANCE.md**
   - Update frequency analysis
   - Per-bot CPU cost estimate
   - Bottleneck identification
   - Optimization opportunities

4. **AI_BEHAVIOR_RECOMMENDATIONS.md**
   - Missing behaviors to add
   - Code to refactor/remove
   - Event system integration plan
   - Priority-ranked fix list

---

## Search Commands

```bash
# Find all Strategy classes
grep -rn "class.*Strategy.*:" src/modules/Playerbot/AI/Strategy --include="*.h"

# Find all Action classes
grep -rn "class.*Action.*:" src/modules/Playerbot/AI/Actions --include="*.h"

# Find all Trigger classes
grep -rn "class.*Trigger.*:" src/modules/Playerbot/AI/Triggers --include="*.h"

# Find AI update loop
grep -rn "void.*Update.*uint32" src/modules/Playerbot/AI --include="*.cpp"

# Find priority calculations
grep -rn "GetPriority\|CalcPriority\|priority" src/modules/Playerbot/AI --include="*.cpp"

# Find state transitions
grep -rn "SetState\|ChangeState\|TransitionTo" src/modules/Playerbot/AI --include="*.cpp"
```

---

## Success Criteria

- [ ] All strategy types documented with purpose and quality rating
- [ ] Action/Trigger binding mechanism fully understood
- [ ] Decision flow traced from trigger to action execution
- [ ] Performance bottlenecks identified
- [ ] Event system integration opportunities documented
- [ ] Missing behaviors identified
- [ ] Prioritized recommendations created

---

## Notes

- This analysis should leverage the new Event System from Combat Refactoring
- Focus on HOW decisions are made, not just WHAT decisions
- Performance is critical - 1000+ bots must be supported
- Integration with Combat Coordinators should be seamless
