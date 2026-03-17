# Gameplay Testing Workflow

## Overview
Specialized workflow for testing and validating PlayerBot gameplay features.

## Workflow Stages

### Stage 1: Bot Behavior Testing
**Agent**: wow-bot-behavior-designer
```yaml
input:
  - test_scenarios: [questing, grinding, following, trading]
  - bot_classes: all
  - test_duration: 30min
output:
  - behavior_score
  - failed_behaviors
  - improvement_suggestions
```

### Stage 2: Combat Mechanics Validation
**Agent**: wow-mechanics-expert
```yaml
input:
  - combat_scenarios: [single_target, aoe, pvp, raid]
  - class_rotations: verify
  - spell_usage: analyze
output:
  - mechanics_accuracy
  - rotation_efficiency
  - spell_timing_issues
```

### Stage 3: PvP Arena Testing
**Agent**: pvp-arena-tactician
```yaml
input:
  - arena_brackets: [2v2, 3v3, 5v5]
  - compositions: test_all
  - strategy_evaluation: true
output:
  - win_rates
  - tactical_errors
  - positioning_analysis
```

### Stage 4: Dungeon & Raid Coordination
**Agent**: wow-dungeon-raid-coordinator
```yaml
input:
  - dungeon_list: [all_classic]
  - raid_list: [current_tier]
  - role_testing: [tank, healer, dps]
output:
  - completion_rates
  - coordination_issues
  - role_performance
```

### Stage 5: Economy Management
**Agent**: wow-economy-manager
```yaml
input:
  - auction_house: test_trading
  - resource_gathering: evaluate
  - gold_management: analyze
output:
  - economic_efficiency
  - trading_patterns
  - resource_optimization
```

### Stage 6: Learning System Evaluation
**Agent**: bot-learning-system
```yaml
input:
  - learning_data: last_7_days
  - adaptation_metrics: analyze
  - performance_trends: evaluate
output:
  - learning_rate
  - adaptation_success
  - behavior_improvements
```

## Execution Schedule

### Daily Gameplay Tests (Off-Peak Hours)
```yaml
schedule:
  02:00: Quick behavior test (15 min)
  03:00: Combat mechanics check (30 min)
  04:00: Economy snapshot (10 min)
```

### Weekly Deep Tests (Weekend)
```yaml
saturday:
  10:00: Full PvP arena testing (2 hours)
  14:00: Dungeon coordination test (3 hours)
  
sunday:
  10:00: Raid simulation (4 hours)
  15:00: Learning system evaluation (1 hour)
```

## Integration with Code Review

### Trigger Conditions
- After bot AI code changes → Run behavior tests
- After combat system changes → Run mechanics tests
- After economy code changes → Run economy tests
- After major updates → Run full gameplay suite

### Metrics to Track
```json
{
  "behavior_accuracy": 95,
  "combat_efficiency": 90,
  "pvp_win_rate": 50,
  "dungeon_success_rate": 95,
  "raid_coordination": 85,
  "economy_balance": 100,
  "learning_improvement": 5
}
```

## Automated Actions

### Critical Gameplay Issues
```yaml
triggers:
  - bot_stuck_rate > 10%: 
      action: rollback_ai_changes
  - combat_failure > 50%:
      action: disable_affected_classes
  - economy_exploit_detected:
      action: immediate_hotfix
```

### Performance Degradation
```yaml
triggers:
  - behavior_score < 70:
      action: increase_learning_rate
  - pvp_win_rate < 30:
      action: review_tactical_logic
  - dungeon_wipes > 5:
      action: adjust_coordination_params
```

## Reporting

### Gameplay Dashboard
- Real-time bot performance metrics
- Class distribution and success rates
- Economic indicators
- Learning curve visualization

### Weekly Gameplay Report
```markdown
# PlayerBot Gameplay Report

## Performance Summary
- Active Bots: {count}
- Overall Success Rate: {percentage}
- Top Issues: {list}

## Class Performance
| Class | Success | Issues | Recommendation |
|-------|---------|--------|----------------|
| Warrior | 92% | Rage management | Adjust threshold |
| Mage | 88% | Mana efficiency | Optimize rotation |

## Recommendations
1. {high_priority_fix}
2. {medium_priority_improvement}
3. {low_priority_enhancement}
```

## Command Line Usage

```powershell
# Run specific gameplay test
.\gameplay_test.ps1 -Agent "wow-bot-behavior-designer" -Scenario "questing"

# Run full gameplay suite
.\gameplay_test.ps1 -Suite "full" -Duration "4h"

# Quick validation after changes
.\gameplay_test.ps1 -Quick -Classes "warrior,mage,priest"
```

## Integration Points

### With Main Review Workflow
- Code Review → Triggers → Gameplay Tests
- Gameplay Issues → Automated Fix Agent
- Performance Metrics → Daily Report Generator

### With CI/CD Pipeline
```yaml
on:
  push:
    paths:
      - 'src/game/AI/**.cpp'
      - 'src/game/playerbot/**.cpp'
jobs:
  gameplay_tests:
    runs-on: windows-latest
    steps:
      - run: gameplay_test.ps1 -Quick
```
