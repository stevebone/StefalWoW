# ZenAI Prompt - Session/Lifecycle Analysis

## Kopiere diesen Text direkt in ZenAI:

---

```
Analyze the Bot Session and Lifecycle management system for TrinityCore Playerbot module.

## Project Location
C:\TrinityBots\TrinityCore\src\modules\Playerbot\

## Directories to Analyze
1. src/modules/Playerbot/Lifecycle/ (~60 files, ~15,000 LOC)
2. src/modules/Playerbot/Session/ (~25 files, ~5,000 LOC)

## Critical Questions

### Question 1: Duplicate Managers
Why do BOTH of these exist?
- BotLifecycleManager.cpp (860 lines)
- BotLifecycleMgr.cpp (826 lines)
These have almost identical names. Which is authoritative? Can they be merged?

### Question 2: Duplicate Performance Monitor
Why does BotPerformanceMonitor.cpp exist in BOTH directories?
- Lifecycle/BotPerformanceMonitor.cpp
- Session/BotPerformanceMonitor.cpp
Are they identical? Should they be merged?

### Question 3: Thread Safety
Is the bot spawning system thread-safe?
- Check BotSpawner.cpp for race conditions
- Check BotSpawnOrchestrator.cpp for concurrent access
- Check StartupSpawnOrchestrator.cpp for mass-spawn safety
- Verify mutex usage and lock hierarchy

### Question 4: Memory Management
Are there memory leaks when bots logout or die?
- Check BotSession destruction
- Check DeathRecoveryManager cleanup
- Check object ownership (raw vs smart pointers)
- Verify all allocated objects are freed

### Question 5: State Machine
Is the bot lifecycle state machine correctly implemented?
Expected states: CREATING → LOGGING_IN → INITIALIZING → READY → PLAYING → LOGGING_OUT → DESTROYED
With death branch: PLAYING → DEAD → RESURRECTING → PLAYING
- Are all transitions valid?
- Are there stuck states?
- Is error recovery handled?

### Question 6: Death Handling Fragmentation
Why are there 4 separate files for death handling?
- DeathRecoveryManager.cpp
- DeathHookIntegration.cpp
- CorpsePreventionManager.cpp
- SafeCorpseManager.cpp
Can these be consolidated?

### Question 7: Session Confusion
What is the relationship between:
- BotSession.cpp
- BotSessionManager.cpp
- BotWorldSessionMgr.cpp
Is there redundant functionality?

## Key Files to Read First
1. Lifecycle/BotLifecycleManager.h - Understand main interface
2. Lifecycle/BotLifecycleMgr.h - Compare with above
3. Lifecycle/BotLifecycleState.h - State definitions
4. Lifecycle/BotSpawner.h - Spawning interface
5. Session/BotSession.h - Session interface
6. Session/BotSessionManager.h - Session registry

## Deliverables Required

### 1. ARCHITECTURE_ANALYSIS.md
- Mermaid diagram of component relationships
- Dependency graph
- Data flow for bot creation
- Object ownership model

### 2. DUPLICATE_REPORT.md
- List ALL duplicate/redundant code
- For each duplicate: recommendation (merge/delete/keep)
- Effort estimate for consolidation
- Risk assessment

### 3. THREAD_SAFETY_REPORT.md
- All identified race conditions
- All potential deadlocks
- Lock hierarchy violations
- Specific code locations with line numbers
- Fix recommendations

### 4. MEMORY_REPORT.md
- Potential memory leaks
- Object lifetime issues
- Raw pointer ownership problems
- Smart pointer recommendations
- Specific code locations

### 5. STATE_MACHINE_ANALYSIS.md
- Current state diagram (Mermaid)
- Invalid state transitions found
- Missing error handling
- Stuck state potential
- Recommended improvements

### 6. RECOMMENDATIONS.md
- Priority P0/P1/P2 ranked improvements
- Effort estimates per improvement
- Risk assessment
- Suggested implementation order
- Quick wins (< 2h fixes)

## Analysis Approach
1. First: Create file inventory with LOC counts
2. Second: Map #include dependencies
3. Third: Read manager classes to understand architecture
4. Fourth: Check thread safety in spawning code
5. Fifth: Check memory management in session code
6. Sixth: Validate state machine implementation
7. Finally: Generate all reports

## Success Criteria
- All 85 files analyzed
- All duplicates identified
- All critical race conditions found
- All potential memory leaks documented
- Complete architecture understanding
- Actionable recommendations provided
```

---

## Alternative: Kürzerer Prompt

Falls ZenAI einen kürzeren Prompt bevorzugt:

```
Analyze src/modules/Playerbot/Lifecycle/ and src/modules/Playerbot/Session/ (~85 files, ~20,000 LOC).

Find:
1. Why BotLifecycleManager.cpp AND BotLifecycleMgr.cpp both exist (duplicate?)
2. Race conditions in BotSpawner/BotSpawnOrchestrator
3. Memory leaks in BotSession destruction
4. State machine correctness for bot lifecycle
5. All duplicate code that can be consolidated

Create reports: ARCHITECTURE_ANALYSIS.md, DUPLICATE_REPORT.md, THREAD_SAFETY_REPORT.md, MEMORY_REPORT.md, RECOMMENDATIONS.md
```
