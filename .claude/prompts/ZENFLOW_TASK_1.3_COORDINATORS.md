# Zenflow Task 1.3: Complete Coordinator & Strategy Analysis

## Prompt for Zenflow (English)

```
Analyze ALL existing Combat Coordinators and Strategies for refactoring to an event-driven architecture.

## SCOPE - COMPLETE LIST

### Combat Coordinators (AI/Combat/)
1. src/modules/Playerbot/AI/Combat/ThreatCoordinator.cpp + .h
2. src/modules/Playerbot/AI/Combat/InterruptCoordinator.cpp + .h
3. src/modules/Playerbot/AI/Combat/FormationManager.cpp + .h
4. src/modules/Playerbot/AI/Combat/CrowdControlManager.cpp + .h
5. src/modules/Playerbot/AI/Combat/BotThreatManager.cpp + .h

### Orchestrators (AI/Coordination/)
6. src/modules/Playerbot/AI/Coordination/RaidOrchestrator.cpp + .h
7. src/modules/Playerbot/AI/Coordination/RoleCoordinator.cpp + .h
8. src/modules/Playerbot/AI/Coordination/ZoneOrchestrator.cpp + .h

### Group Coordination (Advanced/)
9. src/modules/Playerbot/Advanced/GroupCoordinator.cpp + .h
10. src/modules/Playerbot/Advanced/TacticalCoordinator.cpp + .h

### Combat Strategies (AI/Strategy/)
11. src/modules/Playerbot/AI/Strategy/Strategy.cpp + .h (BASE CLASS)
12. src/modules/Playerbot/AI/Strategy/SoloCombatStrategy.cpp + .h
13. src/modules/Playerbot/AI/Strategy/GroupCombatStrategy.cpp + .h

## ANALYSIS PER COMPONENT

For each file document:

### A. Algorithm Complexity
- Big-O notation for EVERY public method
- Identify nested loops and their worst-case scenarios
- Mark methods with O(n²) or worse as CRITICAL

### B. Update Pattern
- Is Update() called every frame?
- Polling or event-driven?
- Update interval (if throttled)
- What happens per update cycle?

### C. Event Integration
- Does it use HostileEventBus? (src/modules/Playerbot/AI/Combat/HostileEventBus.h)
- Does it use EventDispatcher? (src/modules/Playerbot/Core/Events/EventDispatcher.h)
- Which events are subscribed?
- Which events are published?

### D. Context Awareness
- Does it handle different contexts? (Solo, Group, Dungeon, Raid, Arena, BG)
- How does it detect context changes?
- Does it scale behavior based on context?

### E. Data Structures
- Which containers are used? (map, unordered_map, vector, etc.)
- Is data copied or referenced per frame?
- Cache-friendliness of data structures

### F. TrinityCore API Calls
- ObjectAccessor::GetUnit() calls (EXPENSIVE - ~10μs per call)
- ObjectAccessor::GetPlayer() calls
- Group::GetMembers() iterations
- ThreatManager accesses
- Spell/Aura lookups
- Map/Instance queries

### G. Memory Allocations
- Heap allocations per update (std::string, vector resize, etc.)
- Temporary objects created per frame
- String concatenations in hot paths

### H. Thread Safety
- Which mutexes are used?
- Atomic variables?
- Potential race conditions?
- Lock contention risks?

## CRITICAL QUESTIONS TO ANSWER

1. **Missing Coordinators**: Why is there no DungeonCoordinator, ArenaCoordinator, or BattlegroundCoordinator?
2. **Context Handling**: How do current components handle Dungeon vs Raid vs Arena differently?
3. **Overlap Analysis**: Which components have overlapping responsibilities?
4. **Dependency Graph**: Which components depend on which others?

## OUTPUT FORMAT

Create a section for each component:

```markdown
## [Component Name]

### Overview
- File: [Path]
- LOC: [Lines]
- Type: [Coordinator/Orchestrator/Strategy]
- Main purpose: [Description]

### Complexity Analysis
| Method | Complexity | Critical? | Notes |
|--------|------------|-----------|-------|
| Method1 | O(n) | ❌ | ... |
| Method2 | O(n²) | ⚠️ | ... |
| Method3 | O(b×t) | 🔴 | ... |

### Performance Hotspots
1. [Hotspot 1]: [Description + line reference]
2. [Hotspot 2]: [Description + line reference]

### Context Support
| Context | Supported? | How? |
|---------|------------|------|
| Solo | ✅/❌ | ... |
| Group | ✅/❌ | ... |
| Dungeon | ✅/❌ | ... |
| Raid | ✅/❌ | ... |
| Arena | ✅/❌ | ... |
| Battleground | ✅/❌ | ... |

### Event Integration Status
- HostileEventBus: ✅/❌
- EventDispatcher: ✅/❌
- Current pattern: [Polling/Event-driven/Hybrid]

### TrinityCore API Calls (per update)
- ObjectAccessor::GetUnit: ~[N] calls
- Group iteration: ~[N] iterations
- ThreatManager access: ~[N] calls

### Refactoring Recommendations
1. [Recommendation 1 with code example]
2. [Recommendation 2 with code example]

### Migration to Event-driven Architecture
[Concrete plan how this component can be converted to events]
```

## SUMMARY SECTIONS

### 1. Architecture Gap Analysis
- What contexts are NOT properly supported?
- What coordinators are MISSING?
- What SHOULD exist but doesn't?

### 2. Dependency Graph
Create a visual dependency graph showing which components call which.

### 3. Performance Ranking
Rank all 13 components by performance impact (worst first).

### 4. Refactoring Priorities
| Priority | Component | Reason | Effort |
|----------|-----------|--------|--------|
| P0 | ... | ... | ... |
| P1 | ... | ... | ... |

### 5. Quick Wins
Simple fixes with big impact that can be done immediately.

### 6. Breaking Changes
What will require API changes when migrating.

### 7. New Components Needed
Based on the gap analysis, what new coordinators should be created:
- DungeonCoordinator?
- ArenaCoordinator?
- BattlegroundCoordinator?

## CONTEXT

Goal is migration to an event-driven architecture for 5000 concurrent bots.
The new architecture is documented in: .claude/architecture/COMBAT_SYSTEM_ENTERPRISE_SPEC.md

The new interfaces are defined in:
- .claude/architecture/interfaces/CombatSystemInterfaces.h
- .claude/architecture/interfaces/ClassRoleResolver.h

All components must implement appropriate interfaces and become event-driven.
The system must support: Solo, Group (5-man), Dungeon, Raid (10-40), Arena (2v2, 3v3, 5v5), Battleground.
```
