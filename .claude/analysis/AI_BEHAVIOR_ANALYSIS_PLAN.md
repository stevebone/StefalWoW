# AI/Behavior System Analysis - Enterprise Grade Master Plan

**Date**: 2026-01-27  
**Analyst**: Claude (übernommen von Zenflow)  
**Status**: IN PROGRESS

---

## 🎯 Analysis Scope

Das AI/Behavior System ist das "Gehirn" der Bots - es entscheidet:
- **WAS** der Bot tut (Actions)
- **WANN** er es tut (Triggers)
- **WIE** er priorisiert (Strategies, Decision Systems)
- **WIE** er lernt (Learning/Adaptation)

---

## 📊 System Overview (Erste Erkundung)

### Discovered Components

| Category | Directory | Files | Purpose |
|----------|-----------|-------|---------|
| **Core AI** | AI/ | BotAI, HybridAIController, BehaviorManager | Haupt-AI-Logik |
| **Strategies** | AI/Strategy/ | 8 Strategies | Verhaltens-Modi |
| **Actions** | AI/Actions/ | Action, CommonActions, etc. | Ausführbare Befehle |
| **Triggers** | AI/Triggers/ | Trigger | Auslöser für Actions |
| **Decision** | AI/Decision/ | ActionPriorityQueue, BehaviorTree, DecisionFusionSystem | Entscheidungs-Systeme |
| **BehaviorTree** | AI/BehaviorTree/ | BehaviorTree, Nodes/ | Hierarchische Entscheidungen |
| **Utility AI** | AI/Utility/ | UtilitySystem, Evaluators/ | Score-basierte Entscheidungen |
| **Combat** | AI/Combat/ | 30+ Manager | Kampf-spezifische Logik |
| **CombatBehaviors** | AI/CombatBehaviors/ | AoE, Cooldown, Dispel, Interrupt | Kampf-Verhaltens-Manager |
| **ClassAI** | AI/ClassAI/ | 12 Klassen-Verzeichnisse | Klassen-spezifische AI |
| **Learning** | AI/Learning/ | AdaptiveDifficulty, BehaviorAdaptation | Lernende AI |
| **Coordination** | AI/Coordination/ | Arena, BG, Dungeon, Raid | Gruppen-Koordination |
| **Services** | AI/Services/ | HealingTargetSelector, ThreatAssistant | Gemeinsame Services |
| **Integration** | AI/Integration/ | IntegratedAIContext | System-Integration |
| **Cache** | AI/Cache/ | AuraStateCache | Performance-Caching |
| **Blackboard** | AI/Blackboard/ | SharedBlackboard | Geteilter State |
| **Values** | AI/Values/ | Value | Werte-System |

---

## 📋 Analysis Task Plan

### Phase 1: Core Architecture Analysis (2h)
- [x] Task 1.1: BotAI Core Analysis
- [ ] Task 1.2: HybridAIController Analysis
- [ ] Task 1.3: BehaviorManager & BehaviorPriorityManager
- [ ] Task 1.4: Update Loop & Performance

### Phase 2: Decision Systems Analysis (2h)
- [ ] Task 2.1: Strategy System
- [ ] Task 2.2: Action System
- [ ] Task 2.3: Trigger System
- [ ] Task 2.4: DecisionFusionSystem
- [ ] Task 2.5: ActionPriorityQueue
- [ ] Task 2.6: BehaviorTree System
- [ ] Task 2.7: Utility AI System

### Phase 3: Combat Systems Analysis (1.5h)
- [ ] Task 3.1: Combat Manager Inventory
- [ ] Task 3.2: CombatBehaviors Analysis
- [ ] Task 3.3: Combat Integration with Core AI

### Phase 4: ClassAI & Specialization Analysis (1h)
- [ ] Task 4.1: ClassAI Base Classes
- [ ] Task 4.2: Class-specific Implementations Sample
- [ ] Task 4.3: Specialization System

### Phase 5: Learning & Adaptation Analysis (30min)
- [ ] Task 5.1: Learning System Components
- [ ] Task 5.2: Adaptation Mechanisms

### Phase 6: Coordination Analysis (30min)
- [ ] Task 6.1: Coordination Systems
- [ ] Task 6.2: Integration with Combat Refactoring

### Phase 7: Performance & Quality Analysis (1h)
- [ ] Task 7.1: Performance Bottlenecks
- [ ] Task 7.2: Code Quality Issues
- [ ] Task 7.3: Missing Features

### Phase 8: Recommendations & Roadmap (1h)
- [ ] Task 8.1: Prioritized Recommendations
- [ ] Task 8.2: Integration with Event System
- [ ] Task 8.3: Implementation Roadmap

---

## 📁 Deliverables

1. `AI_BEHAVIOR_ARCHITECTURE.md` - Complete system architecture
2. `AI_BEHAVIOR_INVENTORY.md` - All components with quality ratings
3. `AI_BEHAVIOR_DECISION_FLOW.md` - Decision flow from trigger to action
4. `AI_BEHAVIOR_PERFORMANCE.md` - Performance analysis
5. `AI_BEHAVIOR_RECOMMENDATIONS.md` - Prioritized fix list

---

## Current Progress

### ✅ Task 1.1: BotAI Core Analysis - COMPLETE

**File**: `BotAI.h` (1113 lines)

**Key Findings:**

1. **Clean Update Architecture**
   - Single entry point: `UpdateAI(uint32 diff)`
   - Virtual hooks: `OnCombatUpdate()`, `OnNonCombatUpdate()`
   - NO throttling on main update (preserves movement smoothness)

2. **State Machine**
   ```cpp
   enum class BotAIState {
       SOLO, COMBAT, DEAD, TRAVELLING, 
       QUESTING, GATHERING, TRADING, 
       FOLLOWING, FLEEING, RESTING
   };
   ```

3. **Strategy System**
   - `AddStrategy()`, `RemoveStrategy()`, `ActivateStrategy()`
   - `BehaviorPriorityManager` for priority-based selection

4. **Action System**
   - `ExecuteAction(string)`, `QueueAction(Action)`
   - Action queue with context

5. **Event-Driven (IEventHandler)**
   - Implements 12 event handlers:
     - LootEvent, QuestEvent, CombatEvent, CooldownEvent
     - AuraEvent, ResourceEvent, SocialEvent, AuctionEvent
     - NPCEvent, InstanceEvent, GroupEvent, ProfessionEvent

6. **Manager Delegation (Phase 6/7)**
   - `IGameSystemsManager* _gameSystems` - Facade for 48 managers
   - Includes: HybridAI, DecisionFusion, BehaviorTree, etc.

7. **Instance-Only Mode**
   - Lightweight mode for BG/LFG JIT bots
   - Skips: Questing, Professions, AH, Banking

**Quality Rating: ⭐⭐⭐⭐⭐ (5/5)**
- Clean architecture
- Well-documented
- Event-driven
- Performance-conscious

---

## Next Task: 1.2 HybridAIController Analysis
