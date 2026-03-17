# AI/Behavior System - Component Inventory

**Date**: 2026-01-27

---

## 1. Core AI Components

| Component | File | Lines | Quality | Description |
|-----------|------|-------|---------|-------------|
| BotAI | BotAI.h/cpp | 1113+ | ⭐⭐⭐⭐⭐ | Central AI controller |
| HybridAIController | HybridAIController.h/cpp | 209 | ⭐⭐⭐⭐⭐ | Utility AI + BehaviorTree hybrid |
| BehaviorManager | BehaviorManager.h/cpp | ~200 | ⭐⭐⭐⭐ | Behavior coordination |
| BehaviorPriorityManager | BehaviorPriorityManager.h/cpp | 264 | ⭐⭐⭐⭐⭐ | Priority-based strategy selection |
| EnhancedBotAI | EnhancedBotAI.h/cpp | ~300 | ⭐⭐⭐⭐ | Extended AI features |

---

## 2. Decision Systems

| Component | File | Lines | Quality | Description |
|-----------|------|-------|---------|-------------|
| DecisionFusionSystem | Decision/DecisionFusionSystem.h/cpp | 323 | ⭐⭐⭐⭐⭐ | 5-system weighted voting |
| ActionPriorityQueue | Decision/ActionPriorityQueue.h/cpp | ~200 | ⭐⭐⭐⭐ | Spell priority management |
| BehaviorTree | Decision/BehaviorTree.h/cpp | ~300 | ⭐⭐⭐⭐ | Decision tree (separate from BehaviorTree/) |
| ActionScoringEngine | Common/ActionScoringEngine.h/cpp | ~250 | ⭐⭐⭐⭐⭐ | Utility-based action scoring |
| CombatContextDetector | Common/CombatContextDetector.h/cpp | ~150 | ⭐⭐⭐⭐ | Combat situation analysis |

---

## 3. Strategies

| Strategy | File | Quality | Triggers | Actions | Description |
|----------|------|---------|----------|---------|-------------|
| Strategy (Base) | Strategy/Strategy.h/cpp | ⭐⭐⭐⭐⭐ | - | - | Base class with throttling |
| CombatStrategy | Strategy/Strategy.h | ⭐⭐⭐⭐⭐ | Combat | Attack | Combat base class |
| SocialStrategy | Strategy/Strategy.h | ⭐⭐⭐⭐ | Social | Chat | Social base class |
| SoloCombatStrategy | Strategy/SoloCombatStrategy.h/cpp | ⭐⭐⭐⭐ | Health, Combat | Attack, Flee | Solo mob fighting |
| GroupCombatStrategy | Strategy/GroupCombatStrategy.h/cpp | ⭐⭐⭐⭐ | Combat, Role | Assist, Heal | Group combat |
| GrindStrategy | Strategy/GrindStrategy.h/cpp | ⭐⭐⭐⭐ | Distance | Move, Attack | XP grinding |
| QuestStrategy | Strategy/QuestStrategy.h/cpp | ⭐⭐⭐⭐ | Quest | Accept, Complete | Quest completion |
| LootStrategy | Strategy/LootStrategy.h/cpp | ⭐⭐⭐⭐ | Loot | Pickup | Looting |
| RestStrategy | Strategy/RestStrategy.h/cpp | ⭐⭐⭐⭐ | Health, Mana | Eat, Drink | Regeneration |
| SoloStrategy | Strategy/SoloStrategy.h/cpp | ⭐⭐⭐⭐ | Multiple | Multiple | Autonomous play |

---

## 4. Actions

| Action | File | Quality | Type | Description |
|--------|------|---------|------|-------------|
| Action (Base) | Actions/Action.h/cpp | ⭐⭐⭐⭐⭐ | Base | Base with prereqs, chaining |
| MovementAction | Actions/Action.h | ⭐⭐⭐⭐⭐ | Movement | Pathfinding movement |
| CombatAction | Actions/Action.h | ⭐⭐⭐⭐⭐ | Combat | Combat base |
| SpellAction | Actions/Action.h | ⭐⭐⭐⭐⭐ | Combat | Spell casting |
| CommonActions | Actions/CommonActions.h/cpp | ⭐⭐⭐⭐ | Multiple | Common action implementations |
| SpellInterruptAction | Actions/SpellInterruptAction.h/cpp | ⭐⭐⭐⭐ | Combat | Interrupt spells |
| TargetAssistAction | Actions/TargetAssistAction.h/cpp | ⭐⭐⭐⭐ | Combat | Target assist |

---

## 5. Triggers

| Trigger | File | Quality | Type | Description |
|---------|------|---------|------|-------------|
| Trigger (Base) | Triggers/Trigger.h/cpp | ⭐⭐⭐⭐⭐ | Base | Base with conditions |
| HealthTrigger | Triggers/Trigger.h | ⭐⭐⭐⭐⭐ | Health | Health threshold |
| CombatTrigger | Triggers/Trigger.h | ⭐⭐⭐⭐⭐ | Combat | Combat state |
| TimerTrigger | Triggers/Trigger.h | ⭐⭐⭐⭐⭐ | Timer | Interval-based |
| DistanceTrigger | Triggers/Trigger.h | ⭐⭐⭐⭐⭐ | Distance | Distance check |
| QuestTrigger | Triggers/Trigger.h | ⭐⭐⭐⭐ | Quest | Quest state |

---

## 6. BehaviorTree Components

| Component | File | Quality | Description |
|-----------|------|---------|-------------|
| BehaviorTree | BehaviorTree/BehaviorTree.h | ⭐⭐⭐⭐⭐ | Tree container |
| BTBlackboard | BehaviorTree/BehaviorTree.h | ⭐⭐⭐⭐⭐ | Shared data store |
| BTNode | BehaviorTree/BehaviorTree.h | ⭐⭐⭐⭐⭐ | Base node |
| BTComposite | BehaviorTree/BehaviorTree.h | ⭐⭐⭐⭐⭐ | Multi-child node |
| BTSequence | BehaviorTree/BehaviorTree.h | ⭐⭐⭐⭐⭐ | All must succeed |
| BTSelector | BehaviorTree/BehaviorTree.h | ⭐⭐⭐⭐⭐ | First success |
| BTScoredSelector | BehaviorTree/BehaviorTree.h | ⭐⭐⭐⭐⭐ | Utility-based selection |
| BTDecorator | BehaviorTree/BehaviorTree.h | ⭐⭐⭐⭐⭐ | Single-child modifier |
| BTInverter | BehaviorTree/BehaviorTree.h | ⭐⭐⭐⭐⭐ | Invert result |
| BTRepeater | BehaviorTree/BehaviorTree.h | ⭐⭐⭐⭐⭐ | Repeat N times |
| BTCondition | BehaviorTree/BehaviorTree.h | ⭐⭐⭐⭐⭐ | Test condition |
| BTAction | BehaviorTree/BehaviorTree.h | ⭐⭐⭐⭐⭐ | Execute action |
| BehaviorTreeFactory | BehaviorTree/BehaviorTreeFactory.h/cpp | ⭐⭐⭐⭐ | Tree creation |

---

## 7. Utility AI Components

| Component | File | Quality | Description |
|-----------|------|---------|-------------|
| UtilityContext | Utility/UtilitySystem.h | ⭐⭐⭐⭐⭐ | State for evaluation |
| UtilityEvaluator | Utility/UtilitySystem.h | ⭐⭐⭐⭐⭐ | Factor scoring |
| UtilityBehavior | Utility/UtilitySystem.h | ⭐⭐⭐⭐⭐ | Behavior with evaluators |
| UtilityAI | Utility/UtilitySystem.h | ⭐⭐⭐⭐⭐ | Behavior selection |
| UtilityContextBuilder | Utility/UtilityContextBuilder.h | ⭐⭐⭐⭐ | Context construction |

---

## 8. Learning Components

| Component | File | Quality | Description |
|-----------|------|---------|-------------|
| BehaviorAdaptation | Learning/BehaviorAdaptation.h/cpp | ⭐⭐⭐⭐⭐ | ML engine (Q-Learning, Policy Gradients) |
| NeuralNetwork | Learning/BehaviorAdaptation.h | ⭐⭐⭐⭐⭐ | Neural network implementation |
| QFunction | Learning/BehaviorAdaptation.h | ⭐⭐⭐⭐⭐ | Q-value approximator |
| PolicyNetwork | Learning/BehaviorAdaptation.h | ⭐⭐⭐⭐⭐ | Policy gradient network |
| AdaptiveDifficulty | Learning/AdaptiveDifficulty.h/cpp | ⭐⭐⭐⭐ | Difficulty adjustment |
| PerformanceOptimizer | Learning/PerformanceOptimizer.h/cpp | ⭐⭐⭐⭐ | Runtime optimization |
| PlayerPatternRecognition | Learning/PlayerPatternRecognition.h/cpp | ⭐⭐⭐⭐ | Player behavior learning |

---

## 9. Performance Components

| Component | File | Quality | Description |
|-----------|------|---------|-------------|
| AdaptiveAIUpdateThrottler | AdaptiveAIUpdateThrottler.h/cpp | ⭐⭐⭐⭐⭐ | Context-aware throttling |
| GlobalThrottleStatistics | AdaptiveAIUpdateThrottler.h | ⭐⭐⭐⭐⭐ | Global metrics |
| ObjectCache | ObjectCache.h/cpp | ⭐⭐⭐⭐ | Object caching |
| AuraStateCache | Cache/AuraStateCache.h/cpp | ⭐⭐⭐⭐ | Aura state caching |

---

## 10. Integration Components

| Component | File | Quality | Description |
|-----------|------|---------|-------------|
| SharedBlackboard | Blackboard/SharedBlackboard.h/cpp | ⭐⭐⭐⭐⭐ | Thread-safe shared state |
| IntegratedAIContext | Integration/IntegratedAIContext.h/cpp | ⭐⭐⭐⭐ | AI context integration |

---

## 11. Combat Integration (from AI/Combat/)

| Component | File | Quality | Description |
|-----------|------|---------|-------------|
| CombatStateManager | Combat/CombatStateManager.h/cpp | ⭐⭐⭐⭐⭐ | Combat state tracking |
| CombatAIIntegrator | Combat/CombatAIIntegrator.h/cpp | ⭐⭐⭐⭐⭐ | Combat-AI bridge |
| AdaptiveBehaviorManager | Combat/AdaptiveBehaviorManager.h/cpp | ⭐⭐⭐⭐⭐ | Role-based behavior |
| CombatBehaviorIntegration | Combat/CombatBehaviorIntegration.h/cpp | ⭐⭐⭐⭐ | Behavior integration |

---

## Statistics Summary

| Category | Count | Avg Quality |
|----------|-------|-------------|
| Core AI | 5 | ⭐⭐⭐⭐.4 |
| Decision Systems | 5 | ⭐⭐⭐⭐.4 |
| Strategies | 10 | ⭐⭐⭐⭐.2 |
| Actions | 7 | ⭐⭐⭐⭐.4 |
| Triggers | 6 | ⭐⭐⭐⭐.5 |
| BehaviorTree | 13 | ⭐⭐⭐⭐.8 |
| Utility AI | 5 | ⭐⭐⭐⭐.6 |
| Learning | 7 | ⭐⭐⭐⭐.4 |
| Performance | 4 | ⭐⭐⭐⭐.5 |
| Integration | 2 | ⭐⭐⭐⭐.5 |
| Combat Integration | 4 | ⭐⭐⭐⭐.5 |
| **TOTAL** | **68** | **⭐⭐⭐⭐.4** |

---

## Quality Legend

| Rating | Meaning |
|--------|---------|
| ⭐⭐⭐⭐⭐ | Enterprise-grade, no issues |
| ⭐⭐⭐⭐ | Production-ready, minor improvements possible |
| ⭐⭐⭐ | Functional, needs refactoring |
| ⭐⭐ | Problematic, needs significant work |
| ⭐ | Critical issues, needs rewrite |
