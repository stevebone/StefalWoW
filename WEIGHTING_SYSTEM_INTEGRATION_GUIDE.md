# Playerbot Weighting System Integration Guide

## Overview

This guide explains how to integrate the utility-based weighting system with ClassAI specializations. The weighting system enables intelligent multi-criteria decision-making, allowing bots to make human-like choices by evaluating actions across 6 categories.

**Reference Implementation**: `src/modules/Playerbot/AI/ClassAI/Mages/ArcaneMageWeighted.h`

## Quick Start (5 Steps)

1. **Include Headers**
2. **Add ActionScoringEngine Member**
3. **Initialize in Constructor**
4. **Create Scoring Functions**
5. **Replace Rotation Logic**

---

## Step 1: Include Required Headers

```cpp
#include "../../Common/ActionScoringEngine.h"
#include "../../Common/CombatContextDetector.h"
```

---

## Step 2: Add ActionScoringEngine Member

Add to your class definition:

```cpp
class YourSpecRefactored : public SpecializationBase
{
private:
    bot::ai::ActionScoringEngine _scoringEngine;
    uint32 _lastContextUpdate;
    uint32 _contextUpdateInterval; // e.g., 5000ms
};
```

---

## Step 3: Initialize in Constructor

```cpp
YourSpecRefactored(Player* bot)
    : SpecializationBase(bot)
    , _scoringEngine(
        bot::ai::BotRole::RANGED_DPS,  // Or TANK, HEALER, MELEE_DPS
        bot::ai::CombatContext::SOLO)   // Initial context
    , _lastContextUpdate(0)
    , _contextUpdateInterval(5000)
{
    // Enable debug logging if configured
    bool debugLogging = sConfigMgr->GetBoolDefault("Playerbot.AI.Weighting.LogScoring", false);
    _scoringEngine.EnableDebugLogging(debugLogging);
}
```

---

## Step 4: Create Scoring Functions

### 4.1 Main Scoring Entry Point

```cpp
float ScoreAction(
    uint32 actionId,
    bot::ai::ScoringCategory category,
    /* ... spec-specific state parameters ... */) const
{
    using bot::ai::ScoringCategory;

    switch (category)
    {
        case ScoringCategory::SURVIVAL:
            return ScoreSurvival(actionId, healthPercent);

        case ScoringCategory::GROUP_PROTECTION:
            return ScoreGroupProtection(actionId, allyHealth);

        case ScoringCategory::DAMAGE_OPTIMIZATION:
            return ScoreDamage(actionId, resources, enemyCount);

        case ScoringCategory::RESOURCE_EFFICIENCY:
            return ScoreResource(actionId, manaPercent, comboPoints);

        case ScoringCategory::POSITIONING_MECHANICS:
            return ScorePositioning(actionId, range, inMechanic);

        case ScoringCategory::STRATEGIC_VALUE:
            return ScoreStrategic(actionId, fightPhase, cooldownsReady);

        default:
            return 0.0f;
    }
}
```

### 4.2 Category-Specific Scoring Functions

#### Survival Scoring (All Roles)

```cpp
float ScoreSurvival(uint32 actionId, float healthPercent) const
{
    float urgency = (100.0f - healthPercent) / 100.0f; // 0.0-1.0

    switch (actionId)
    {
        case DEFENSIVE_COOLDOWN_1:
            if (healthPercent < 20.0f)
                return 1.0f; // Critical - maximum urgency
            else if (healthPercent < 40.0f)
                return 0.7f * urgency;
            else
                return 0.0f; // Not needed

        case HEAL_SELF:
            return urgency; // Linear scaling with health loss

        case SHIELD:
            if (healthPercent < 60.0f)
                return 0.5f + (0.5f * urgency);
            else
                return 0.0f;

        default:
            return 0.0f; // No survival value
    }
}
```

#### Damage Optimization (DPS Priority)

```cpp
float ScoreDamage(uint32 actionId, uint32 resources, uint32 enemyCount) const
{
    bool isAoE = (enemyCount >= 3);

    switch (actionId)
    {
        case MAJOR_COOLDOWN:
        {
            // Example: Score based on resource state and cooldown readiness
            float resourceValue = static_cast<float>(resources) / GetMaxResource();
            float contextBonus = 0.7f;

            // Higher value in important contexts
            bot::ai::CombatContext context = _scoringEngine.GetContext();
            if (context == bot::ai::CombatContext::RAID_HEROIC ||
                context == bot::ai::CombatContext::DUNGEON_BOSS)
                contextBonus = 1.0f;

            return resourceValue * contextBonus;
        }

        case BUILDER:
        {
            if (resources < GetMaxResource())
                return 0.6f; // Good value when building
            else
                return 0.3f; // Lower value at cap
        }

        case SPENDER:
        {
            if (resources >= GetMaxResource() * 0.8f)
                return 0.9f; // High value near cap
            else if (resources >= GetMaxResource() * 0.5f)
                return 0.5f; // Medium value
            else
                return 0.2f; // Low value, not enough resources
        }

        case AOE_ABILITY:
        {
            if (isAoE && enemyCount >= 5)
                return 0.9f;
            else if (isAoE)
                return 0.6f;
            else
                return 0.1f; // Poor single target
        }

        default:
            return 0.0f;
    }
}
```

#### Resource Efficiency (Healer/Caster Priority)

```cpp
float ScoreResource(uint32 actionId, uint32 manaPercent, uint32 comboPoints) const
{
    switch (actionId)
    {
        case MANA_RECOVERY:
        {
            if (manaPercent < 20)
                return 1.0f; // Critical recovery needed
            else if (manaPercent < 40)
                return 0.6f;
            else
                return 0.0f; // Not needed
        }

        case FREE_PROC_ABILITY:
        {
            // Free cast = perfect efficiency
            if (HasProc())
                return 1.0f;
            else
                return 0.0f; // Should never cast without proc
        }

        case EXPENSIVE_ABILITY:
        {
            if (manaPercent > 60)
                return 0.7f; // Can afford it
            else if (manaPercent > 30)
                return 0.4f; // Risky but possible
            else
                return 0.0f; // Too expensive
        }

        case EFFICIENT_FILLER:
        {
            return 0.8f; // Always good efficiency
        }

        default:
            return 0.5f; // Neutral efficiency
    }
}
```

#### Group Protection (Healer/Tank Priority)

```cpp
float ScoreGroupProtection(uint32 actionId, std::vector<AllyHealth> allies) const
{
    switch (actionId)
    {
        case SINGLE_TARGET_HEAL:
        {
            // Find most urgent ally
            AllyHealth* mostUrgent = FindMostUrgentAlly(allies);
            if (!mostUrgent)
                return 0.0f;

            float urgency = (100.0f - mostUrgent->healthPct) / 100.0f;
            float rolePriority = mostUrgent->isTank ? 2.0f : 1.0f;

            return urgency * rolePriority;
        }

        case AOE_HEAL:
        {
            // Count allies below threshold
            uint32 lowHealthCount = CountAlliesBelowHealth(allies, 70.0f);

            if (lowHealthCount >= 4)
                return 0.9f;
            else if (lowHealthCount >= 2)
                return 0.6f;
            else
                return 0.2f;
        }

        case INTERRUPT:
        {
            if (IsEnemyCastingDangerous())
                return 1.0f; // Maximum urgency
            else if (IsEnemyCasting())
                return 0.6f;
            else
                return 0.0f; // No cast to interrupt
        }

        case DISPEL:
        {
            uint32 debuffCount = CountDispellableDebuffs(allies);
            return std::min(static_cast<float>(debuffCount) * 0.3f, 1.0f);
        }

        default:
            return 0.0f;
    }
}
```

#### Strategic Value (All Roles)

```cpp
float ScoreStrategic(uint32 actionId, uint32 fightPhase, bool cooldownsReady) const
{
    bot::ai::CombatContext context = _scoringEngine.GetContext();

    switch (actionId)
    {
        case MAJOR_COOLDOWN:
        {
            // Save for important fights
            if (context == bot::ai::CombatContext::DUNGEON_BOSS ||
                context == bot::ai::CombatContext::RAID_NORMAL ||
                context == bot::ai::CombatContext::RAID_HEROIC)
                return 1.0f; // Use on bosses

            else if (context == bot::ai::CombatContext::DUNGEON_TRASH)
                return 0.3f; // Save for boss

            else
                return 0.5f; // Use freely in solo/group
        }

        case HEROISM:
        {
            // Only use on critical encounters
            if (context == bot::ai::CombatContext::RAID_HEROIC && fightPhase == BURN_PHASE)
                return 1.0f;
            else if (context == bot::ai::CombatContext::RAID_NORMAL && fightPhase == BURN_PHASE)
                return 0.8f;
            else
                return 0.0f; // Don't waste
        }

        case RESOURCE_GENERATOR:
        {
            // Strategic value of generators before burst
            if (cooldownsReady && GetResourcePercent() < 80)
                return 0.7f; // Prepare for burst
            else
                return 0.3f;
        }

        default:
            return 0.5f; // Neutral strategic value
    }
}
```

---

## Step 5: Replace Rotation Logic

### 5.1 Update Context Periodically

```cpp
void UpdateCombatContext()
{
    uint32 currentTime = getMSTime();
    if (currentTime - _lastContextUpdate < _contextUpdateInterval)
        return; // Not time to update yet

    _lastContextUpdate = currentTime;

    // Detect new context
    bot::ai::CombatContext newContext = bot::ai::CombatContextDetector::DetectContext(this->GetBot());

    // Update scoring engine if context changed
    if (newContext != _scoringEngine.GetContext())
    {
        _scoringEngine.SetContext(newContext);

        TC_LOG_DEBUG("playerbot", "YourSpec: Context changed to {}",
            bot::ai::ActionScoringEngine::GetContextName(newContext));
    }
}
```

### 5.2 Replace Rotation with Weighted Selection

**Before (Order-Based)**:
```cpp
void ExecuteRotation(::Unit* target)
{
    // Old way: Hard-coded priority order
    if (CanUse(MAJOR_COOLDOWN))
        return Use(MAJOR_COOLDOWN);

    if (CanUse(BUILDER))
        return Use(BUILDER);

    if (CanUse(SPENDER))
        return Use(SPENDER);

    // First viable option wins
}
```

**After (Weight-Based)**:
```cpp
void ExecuteWeightedRotation(::Unit* target)
{
    Player* bot = this->GetBot();
    if (!bot)
        return;

    // Gather current state
    uint32 resources = GetCurrentResources();
    uint32 manaPercent = bot->GetPowerPct(POWER_MANA);
    float healthPercent = bot->GetHealthPct();
    uint32 enemyCount = this->GetEnemiesInRange(40.0f);

    // Build list of available actions
    std::vector<uint32> availableActions;

    if (this->IsSpellReady(BUILDER))
        availableActions.push_back(BUILDER);

    if (this->IsSpellReady(SPENDER))
        availableActions.push_back(SPENDER);

    if (this->IsSpellReady(MAJOR_COOLDOWN))
        availableActions.push_back(MAJOR_COOLDOWN);

    // Add defensives if needed
    if (healthPercent < 50.0f)
    {
        if (this->IsSpellReady(DEFENSIVE_CD))
            availableActions.push_back(DEFENSIVE_CD);
    }

    if (availableActions.empty())
        return;

    // Score all available actions
    std::vector<bot::ai::ActionScore> scores = _scoringEngine.ScoreActions(
        availableActions,
        [this, resources, manaPercent, healthPercent, enemyCount]
        (bot::ai::ScoringCategory category, uint32 actionId) -> float
        {
            return ScoreAction(actionId, category, resources, manaPercent, healthPercent, enemyCount);
        }
    );

    // Execute best action
    uint32 bestAction = _scoringEngine.GetBestAction(scores);
    if (bestAction != 0)
    {
        ExecuteAction(bestAction, target);
    }
}
```

---

## Configuration

### Enable Weighting System

Edit `playerbots.conf`:

```ini
# Enable weighting system
Playerbot.AI.Weighting.Enable = 1

# Enable debug logging (optional)
Playerbot.AI.Weighting.LogScoring = 1
Playerbot.AI.Weighting.LogTopActions = 3
```

### Tune Weights (Optional)

Adjust base weights for your spec:

```ini
# Increase damage weight for pure DPS specs
Playerbot.AI.Weighting.DamageWeight = 180

# Increase group protection for healers
Playerbot.AI.Weighting.GroupProtectionWeight = 200
```

---

## Testing Your Integration

### 1. Enable Debug Logging

```ini
Playerbot.AI.Weighting.LogScoring = 1
```

### 2. Check Logs

```
playerbot.weighting: ActionScoringEngine: Scored action 12345 = 287.50
  Role: Ranged DPS, Context: Dungeon Boss
  Category Breakdown:
    Damage               : 245.00 (weight: 270.00)
    Resource             :  42.50 (weight:  85.00)
```

### 3. Verify Context Detection

```cpp
TC_LOG_DEBUG("playerbot", "Current context: {}",
    bot::ai::CombatContextDetector::GetContextDescription(_scoringEngine.GetContext()));
```

Expected output:
```
playerbot: Current context: Dungeon Boss (5-man instance, boss encounter)
```

### 4. Test Decision Quality

Monitor bot decisions in various contexts:
- Solo questing: Should prioritize survival and damage
- Dungeon trash: Should prioritize AoE damage
- Dungeon boss: Should save cooldowns, use defensives wisely
- Raid: Should coordinate with group, follow mechanics

---

## Common Patterns

### Pattern 1: Cooldown with Conditions

```cpp
case MAJOR_OFFENSIVE_CD:
{
    float score = 0.0f;

    // Damage category
    if (category == ScoringCategory::DAMAGE_OPTIMIZATION)
    {
        // Require resources
        if (resources >= GetMaxResource() * 0.8f)
            score += 0.4f;

        // Require target count (for AoE CDs)
        if (enemyCount >= 3)
            score += 0.3f;

        // Context bonus
        bot::ai::CombatContext ctx = _scoringEngine.GetContext();
        if (ctx == bot::ai::CombatContext::RAID_HEROIC)
            score += 0.3f;

        return std::clamp(score, 0.0f, 1.0f);
    }

    return 0.0f;
}
```

### Pattern 2: Emergency Defensive

```cpp
case EMERGENCY_DEFENSIVE:
{
    if (category == ScoringCategory::SURVIVAL)
    {
        if (healthPercent < 15.0f)
            return 1.0f; // Always use when critical

        // Otherwise not needed
        return 0.0f;
    }
    return 0.0f;
}
```

### Pattern 3: Resource Builder

```cpp
case RESOURCE_BUILDER:
{
    if (category == ScoringCategory::DAMAGE_OPTIMIZATION)
    {
        if (resources < GetMaxResource())
            return 0.6f; // Good when building
        else
            return 0.2f; // Low value at cap
    }

    if (category == ScoringCategory::RESOURCE_EFFICIENCY)
    {
        // Efficient builder (low cost)
        return 0.8f;
    }

    return 0.0f;
}
```

### Pattern 4: Heal Target Selection

```cpp
case SINGLE_TARGET_HEAL:
{
    if (category == ScoringCategory::GROUP_PROTECTION)
    {
        // Get most urgent ally
        AllyHealth* target = FindMostUrgentAlly();
        if (!target)
            return 0.0f;

        float urgency = (100.0f - target->healthPct) / 100.0f;

        // Tank priority
        if (target->isTank)
            urgency *= 2.0f;

        return std::clamp(urgency, 0.0f, 1.0f);
    }

    return 0.0f;
}
```

---

## Performance Considerations

### CPU Cost

- **Per Action Scoring**: ~64 operations (~1-2 microseconds)
- **10 Actions per Frame**: ~640 operations (~10-20 microseconds)
- **Expected Overhead**: <5% CPU for bot AI

### Memory Cost

- **ActionScoringEngine**: ~36 bytes per bot
- **5000 Bots**: ~180 KB total (negligible)

### Optimization Tips

1. **Limit Available Actions**:
   ```cpp
   // Only add actions that are actually castable
   if (this->IsSpellReady(SPELL) && HasEnoughMana(SPELL))
       availableActions.push_back(SPELL);
   ```

2. **Cache Context Detection**:
   ```cpp
   // Update context every 5 seconds, not every frame
   if (currentTime - _lastContextUpdate >= 5000)
       UpdateCombatContext();
   ```

3. **Skip Zero Categories**:
   ```cpp
   // Return early for irrelevant categories
   if (category == ScoringCategory::GROUP_PROTECTION && !isHealer && !isTank)
       return 0.0f; // DPS doesn't score group protection
   ```

---

## Troubleshooting

### Issue: Bot always chooses same action

**Cause**: One action scores much higher than others

**Solution**: Review scoring functions, ensure proper value ranges (0.0-1.0)

```cpp
// Bad: Always returns 1.0
return 1.0f;

// Good: Scale based on conditions
return std::clamp(urgency * modifier, 0.0f, 1.0f);
```

### Issue: Bot ignores important actions

**Cause**: Action not in available actions list OR scores too low

**Solution**:
1. Check `IsSpellReady()` conditions
2. Enable debug logging to see scores
3. Adjust category weights or scoring values

### Issue: Performance degradation with many bots

**Cause**: Scoring too many actions per frame

**Solution**:
1. Limit `availableActions` list
2. Skip scoring actions that are on cooldown
3. Increase context update interval

---

## Reference Implementation

See complete example:
- **File**: `src/modules/Playerbot/AI/ClassAI/Mages/ArcaneMageWeighted.h`
- **Lines**: 1-700+ (full spec with scoring)

Key sections:
- Constructor: Lines 39-48
- Context update: Lines 60-75
- Weighted rotation: Lines 80-145
- Scoring functions: Lines 200-520
- Action execution: Lines 550-580

---

## FAQ

**Q: Do I need to convert all specs at once?**
A: No! The system is backward compatible. Convert specs incrementally.

**Q: Can I mix weighted and non-weighted logic?**
A: Yes! Use weighted selection for complex decisions, keep simple logic simple.

**Q: How do I tune weights for my spec?**
A: Start with default weights, enable debug logging, observe bot behavior, adjust scoring functions.

**Q: What if my spec doesn't fit the 6 categories?**
A: Return 0.0f for irrelevant categories. Focus on categories that matter for your spec.

**Q: Can I add custom categories?**
A: Not easily. The 6 categories cover all common scenarios. Use strategic value for spec-specific logic.

---

## Next Steps

1. Choose a spec to integrate (start with one you know well)
2. Follow the 5-step integration pattern
3. Enable debug logging and test in solo content
4. Tune scoring functions based on observed behavior
5. Test in group/dungeon/raid content
6. Disable debug logging for production

**Happy coding!**
