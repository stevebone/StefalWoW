# Behavior Tree Decision-Making & Logging Analysis

**Status**: ✅ **ANALYSIS COMPLETE**
**Date**: 2025-11-09
**Scope**: Behavior Tree system decision-making logic and logging review

---

## Executive Summary

**Key Findings**:
- ✅ Loggers correctly configured (`playerbot.bt`, `playerbot.classai`)
- ⚠️ **NO WEIGHT SYSTEM CURRENTLY IMPLEMENTED**
- ⚠️ **Decision-making is purely sequential, not priority-based**
- ⚠️ **Limited decision visibility - needs enhanced logging**

---

## Current Logger Configuration

### Loggers Used

| Logger Name | Usage | Level | Purpose |
|-------------|-------|-------|---------|
| `playerbot.bt` | BehaviorTreeFactory.cpp | ERROR, INFO | Tree creation errors, custom tree registration |
| `playerbot.classai` | ClassBehaviorTreeRegistry.cpp | INFO, ERROR | Class tree registration, lookup failures |

### Logging Locations

**BehaviorTreeFactory.cpp**:
```cpp
// Line 54: Error logging
TC_LOG_ERROR("playerbot.bt", "Unknown tree type requested: {}", uint32(type));

// Line 562: Registration logging
TC_LOG_INFO("playerbot.bt", "Registered custom behavior tree: {}", name);

// Line 573: Lookup error
TC_LOG_ERROR("playerbot.bt", "Custom tree '{}' not found in registry", name);
```

**ClassBehaviorTreeRegistry.cpp**:
```cpp
// Line 40: Registration logging
TC_LOG_INFO("playerbot.classai", "Registered behavior tree for class {} spec {}",
    static_cast<uint8>(classId), specId);

// Line 53: Lookup error
TC_LOG_ERROR("playerbot.classai", "No behavior tree found for class {} spec {}",
    static_cast<uint8>(classId), specId);

// Line 71, 87: Initialization logging
TC_LOG_INFO("playerbot.classai", "Initializing class behavior trees for 13 classes...");
TC_LOG_INFO("playerbot.classai", "Class behavior tree initialization complete ({} trees registered)",
    _treeBuilders.size());
```

### ✅ Logger Verification Status

**Configuration**: ✅ Correct - Both loggers already exist in codebase
- `playerbot.bt` - Used for behavior tree factory operations
- `playerbot.classai` - Used for class-specific tree registration

**No Config Changes Needed**: These loggers inherit from `Logger.root` correctly

---

## Decision-Making Analysis

### Current Implementation: Sequential Execution

The behavior tree uses **sequential decision-making** without weights or priorities:

#### 1. Selector Nodes (Priority by Order)

**How it works**:
```cpp
class BTSelector : public BTComposite
{
    BTStatus Tick(BotAI* ai, BTBlackboard& blackboard) override
    {
        while (_currentChild < _children.size())
        {
            BTStatus status = _children[_currentChild]->Tick(ai, blackboard);

            if (status == BTStatus::SUCCESS)  // ← FIRST SUCCESS WINS
            {
                _status = BTStatus::SUCCESS;
                Reset();
                return _status;
            }

            // Try next child only if this one failed
            _currentChild++;
        }

        // All children failed
        _status = BTStatus::FAILURE;
        return _status;
    }
};
```

**Decision Logic**:
- ⚠️ **Order-based priority ONLY**
- First child that succeeds wins
- No scoring, no weights
- No evaluation of "best" option
- Cannot choose between multiple viable options

**Example** (Melee Combat Tree):
```cpp
auto root = std::make_shared<BTSelector>("MeleeCombatRoot");

// Branch 1: Flee if critically wounded (HIGHEST PRIORITY - checked first)
root->AddChild(fleeCondition);

// Branch 2: Melee combat (LOWER PRIORITY - checked second)
root->AddChild(combatSequence);
```

**Priority**: Determined by **insertion order**, not weights

---

#### 2. Sequence Nodes (All Must Succeed)

**How it works**:
```cpp
class BTSequence : public BTComposite
{
    BTStatus Tick(BotAI* ai, BTBlackboard& blackboard) override
    {
        while (_currentChild < _children.size())
        {
            BTStatus status = _children[_currentChild]->Tick(ai, blackboard);

            if (status == BTStatus::FAILURE)  // ← ANY FAILURE ABORTS
            {
                _status = BTStatus::FAILURE;
                Reset();
                return _status;
            }

            // Continue to next child
            _currentChild++;
        }

        // All succeeded
        _status = BTStatus::SUCCESS;
        return _status;
    }
};
```

**Decision Logic**:
- All children must succeed in order
- First failure aborts the sequence
- No weights, no priorities
- Pure sequential AND logic

---

### ⚠️ CRITICAL ISSUE: No Weight/Scoring System

**Problem**: The current system cannot make **informed decisions** when multiple actions are viable

**Example Scenario**:
```cpp
// Current implementation - no ability to choose "best" heal target
auto healSeq = std::make_shared<BTSequence>("HolyHeal");
healSeq->AddChild(std::make_shared<BTFindWoundedAlly>());  // ← Finds FIRST wounded ally
healSeq->AddChild(std::make_shared<BTCastHeal>());         // ← Heals whoever was found first
```

**Missing Capability**:
- ❌ Cannot evaluate "most wounded" ally
- ❌ Cannot prioritize tank over DPS
- ❌ Cannot score targets by multiple criteria
- ❌ Cannot choose between multiple valid spell options based on situation

---

## Recommended Weight/Priority System

### Proposed: Utility-Based AI with Scoring

Add a scoring system for intelligent decision-making:

```cpp
/**
 * @brief Scored Selector - evaluates all children and picks highest score
 */
class BTScoredSelector : public BTComposite
{
public:
    using ScorerFunc = std::function<float(BotAI*, BTBlackboard&)>;

    void AddChild(std::shared_ptr<BTNode> child, ScorerFunc scorer)
    {
        _children.push_back(child);
        _scorers.push_back(scorer);
    }

    BTStatus Tick(BotAI* ai, BTBlackboard& blackboard) override
    {
        if (_children.empty())
            return BTStatus::FAILURE;

        // EVALUATE ALL OPTIONS
        float bestScore = -FLT_MAX;
        size_t bestIndex = 0;

        for (size_t i = 0; i < _children.size(); ++i)
        {
            float score = _scorers[i](ai, blackboard);

            TC_LOG_TRACE("playerbot.bt.scoring",
                "Child {} ('{}') scored: {:.2f}",
                i, _children[i]->GetName(), score);

            if (score > bestScore)
            {
                bestScore = score;
                bestIndex = i;
            }
        }

        TC_LOG_DEBUG("playerbot.bt.decision",
            "Selected child {} ('{}') with score {:.2f}",
            bestIndex, _children[bestIndex]->GetName(), bestScore);

        // Execute highest-scoring child
        return _children[bestIndex]->Tick(ai, blackboard);
    }

private:
    std::vector<ScorerFunc> _scorers;
};
```

### Usage Example: Intelligent Healing

```cpp
auto healSelector = std::make_shared<BTScoredSelector>("SmartHealSelection");

// Option 1: Heal tank (HIGH PRIORITY)
healSelector->AddChild(
    std::make_shared<BTHealTank>(),
    [](BotAI* ai, BTBlackboard& bb) -> float {
        Unit* tank = FindTank();
        if (!tank || tank->GetHealthPct() > 80.0f)
            return 0.0f;  // Not needed

        // Score inversely proportional to health
        float urgency = (100.0f - tank->GetHealthPct()) / 100.0f;
        float tankBonus = 2.0f;  // Tanks are 2x priority
        return urgency * tankBonus * 100.0f;  // 0-200 range
    }
);

// Option 2: Heal DPS (MEDIUM PRIORITY)
healSelector->AddChild(
    std::make_shared<BTHealDPS>(),
    [](BotAI* ai, BTBlackboard& bb) -> float {
        Unit* dps = FindMostWoundedDPS();
        if (!dps || dps->GetHealthPct() > 70.0f)
            return 0.0f;

        float urgency = (100.0f - dps->GetHealthPct()) / 100.0f;
        return urgency * 100.0f;  // 0-100 range
    }
);

// Option 3: AoE heal (if multiple wounded)
healSelector->AddChild(
    std::make_shared<BTAoEHeal>(),
    [](BotAI* ai, BTBlackboard& bb) -> float {
        uint32 woundedCount = CountWoundedAllies(80.0f);
        if (woundedCount < 3)
            return 0.0f;

        return woundedCount * 30.0f;  // More wounded = higher score
    }
);
```

**Decision Output**:
```
[TRACE] playerbot.bt.scoring: Child 0 ('HealTank') scored: 120.00
[TRACE] playerbot.bt.scoring: Child 1 ('HealDPS') scored: 45.00
[TRACE] playerbot.bt.scoring: Child 2 ('AoEHeal') scored: 90.00
[DEBUG] playerbot.bt.decision: Selected child 0 ('HealTank') with score 120.00
```

---

## Enhanced Logging Recommendations

### 1. Add Decision Logging

**Current Problem**: No visibility into **why** a decision was made

**Recommendation**: Add logging at decision points

```cpp
// In BTSelector::Tick()
BTStatus Tick(BotAI* ai, BTBlackboard& blackboard) override
{
    TC_LOG_TRACE("playerbot.bt.execution",
        "Evaluating Selector '{}' with {} children",
        GetName(), _children.size());

    while (_currentChild < _children.size())
    {
        TC_LOG_TRACE("playerbot.bt.execution",
            "Trying child {} ('{}') in Selector '{}'",
            _currentChild, _children[_currentChild]->GetName(), GetName());

        BTStatus status = _children[_currentChild]->Tick(ai, blackboard);

        if (status == BTStatus::SUCCESS)
        {
            TC_LOG_DEBUG("playerbot.bt.decision",
                "Selector '{}' succeeded via child {} ('{}')",
                GetName(), _currentChild, _children[_currentChild]->GetName());

            _status = BTStatus::SUCCESS;
            Reset();
            return _status;
        }

        TC_LOG_TRACE("playerbot.bt.execution",
            "Child {} failed, trying next", _currentChild);

        _currentChild++;
    }

    TC_LOG_DEBUG("playerbot.bt.decision",
        "Selector '{}' failed - all children failed", GetName());

    _status = BTStatus::FAILURE;
    return _status;
}
```

### 2. Add Performance Logging

```cpp
class BTNode
{
protected:
    void LogPerformance(uint32 executionTimeMs)
    {
        if (executionTimeMs > 10)  // Log slow nodes
        {
            TC_LOG_WARN("playerbot.bt.performance",
                "Slow node '{}' took {}ms to execute",
                _name, executionTimeMs);
        }
    }
};
```

### 3. Add Blackboard Change Logging

```cpp
template<typename T>
void Set(std::string const& key, T const& value)
{
    bool isNew = (_data.find(key) == _data.end());
    _data[key] = value;

    TC_LOG_TRACE("playerbot.bt.blackboard",
        "Blackboard {}: {} = {}",
        isNew ? "SET" : "UPDATE", key, value);
}
```

---

## Configuration Needed for Enhanced Logging

### Add to worldserver.conf.dist

```conf
# Behavior Tree logging (optional - inherits from root by default)
#Logger.playerbot.bt=6,Console Server                    # All BT factory operations (TRACE)
#Logger.playerbot.bt.execution=6,Console Server          # Node execution flow (TRACE)
#Logger.playerbot.bt.decision=3,Console Server           # Decision logging (INFO)
#Logger.playerbot.bt.scoring=6,Console Server            # Scoring details (TRACE)
#Logger.playerbot.bt.performance=2,Console Server        # Performance warnings (WARN)
#Logger.playerbot.bt.blackboard=6,Console Server         # Blackboard changes (TRACE)
#Logger.playerbot.classai=3,Console Server               # Class tree registration (INFO)

# Production configuration (minimal logging)
Logger.playerbot.bt=2,Server                             # WARN level, file only
Logger.playerbot.classai=2,Server                        # WARN level, file only

# Development configuration (full visibility)
#Logger.playerbot.bt=6,Console Server                    # TRACE level, console + file
#Logger.playerbot.bt.execution=6,Console Server
#Logger.playerbot.bt.decision=3,Console Server
```

---

## Weight/Priority Comparison

### Current System (Order-Based)

**Strengths**:
- ✅ Simple and predictable
- ✅ Easy to understand
- ✅ Fast execution (no evaluation overhead)

**Weaknesses**:
- ❌ Cannot make intelligent choices
- ❌ First viable option always wins
- ❌ Cannot adapt to changing situations
- ❌ Hard to tune behavior dynamically

**Example**:
```cpp
// Heal priority: ALWAYS tank first, even if DPS is dying
root->AddChild(healTank);   // Checked first
root->AddChild(healDPS);    // Only if tank is full health
```

---

### Proposed System (Score-Based)

**Strengths**:
- ✅ Intelligent decision-making
- ✅ Multi-criteria evaluation
- ✅ Adapts to situation
- ✅ Easy to tune with weight adjustments
- ✅ Better debugging (can see scores)

**Weaknesses**:
- ⚠️ More CPU overhead (evaluates all options)
- ⚠️ More complex to implement
- ⚠️ Requires careful score design

**Example**:
```cpp
// Smart healing: Tank at 50% health scores 100
//                DPS at 20% health scores 80
// → Heals tank (higher score)

// But if DPS is at 10% health, scores 90
// → Still heals tank (100 > 90)

// But if tank is at 90% health, scores 20
// DPS at 20% health scores 80
// → Heals DPS (80 > 20)
```

---

## Recommendations

### Immediate Actions

1. **✅ Logging is correct** - No changes needed for existing loggers
   - `playerbot.bt` and `playerbot.classai` are properly configured
   - Inheriting from root logger works correctly

2. **⚠️ Add weight/scoring system** - PRIORITY RECOMMENDATION
   - Implement `BTScoredSelector` for intelligent decisions
   - Start with healing (most critical for utility-based AI)
   - Add logging to show scoring process

3. **⚠️ Enhance decision logging** - MEDIUM PRIORITY
   - Add TRACE logging to show node execution flow
   - Add DEBUG logging to show decision rationale
   - Add performance warnings for slow nodes

### Long-Term Improvements

1. **Blackboard logging** - For debugging state changes
2. **Performance profiling** - Identify bottleneck nodes
3. **Visual debugging** - Export tree execution to graphviz
4. **Dynamic weight tuning** - Allow runtime adjustment of scores

---

## Example: Full Decision Flow with Logging

**Scenario**: Bot needs to decide what to do in combat

**Current System** (no logging, sequential):
```
[Silent execution - no visibility]
1. Check flee condition → FAIL
2. Check combat sequence → SUCCESS
   2a. Has target? → SUCCESS
   2b. In range? → SUCCESS
   2c. Cast spell → SUCCESS
```

**Proposed System** (with scoring and logging):
```
[TRACE] playerbot.bt.execution: Evaluating Selector 'CombatRoot' with 3 children
[TRACE] playerbot.bt.scoring: Child 0 ('FleeToSafety') scored: 0.00 (health 85%)
[TRACE] playerbot.bt.scoring: Child 1 ('UseDefensiveCooldown') scored: 15.00 (health 85%, no imminent threat)
[TRACE] playerbot.bt.scoring: Child 2 ('OffensiveRotation') scored: 90.00 (target available, in range, resources available)
[DEBUG] playerbot.bt.decision: Selected child 2 ('OffensiveRotation') with score 90.00
[INFO]  playerbot.bt: Executing offensive rotation against Hogger
```

---

## Conclusion

### Current Status

✅ **Loggers are correct** - Both `playerbot.bt` and `playerbot.classai` properly configured
⚠️ **No weight system** - Decisions are purely sequential (first success wins)
⚠️ **Limited visibility** - Minimal logging makes debugging difficult

### Critical Finding

**The behavior tree does NOT use weights or priorities** - it uses **insertion order** to determine priority. This means:

- First child added to Selector = highest priority
- No ability to choose "best" option from multiple viable options
- No scoring or evaluation system

### Recommendations

1. **CRITICAL**: Implement utility-based AI with scoring system
2. **HIGH**: Add enhanced decision logging for debugging
3. **MEDIUM**: Add performance monitoring
4. **LOW**: Add blackboard change logging

---

**Document Version**: 1.0
**Last Updated**: 2025-11-09
**Analysis By**: Claude (Anthropic AI Assistant)
**Status**: BEHAVIOR TREE ANALYSIS COMPLETE
