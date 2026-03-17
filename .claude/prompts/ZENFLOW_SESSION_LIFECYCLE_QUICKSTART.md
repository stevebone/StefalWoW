# ZenAI Session/Lifecycle Analysis - Quick Start

## 🚀 ZENAI COMMAND

```
@zenai analyze

Analyze the Bot Session and Lifecycle management system for TrinityBots Playerbot.

## Scope
- src/modules/Playerbot/Lifecycle/ (~60 files)
- src/modules/Playerbot/Session/ (~25 files)

## Primary Questions
1. Why do both BotLifecycleManager.cpp AND BotLifecycleMgr.cpp exist? (860 vs 826 lines, almost same name!)
2. Why does BotPerformanceMonitor.cpp exist in BOTH Lifecycle/ AND Session/?
3. Are there race conditions in the spawning system?
4. Are there memory leaks when bots logout or die?
5. Is the state machine (CREATING→READY→PLAYING→DEAD→DESTROYED) correctly implemented?

## Known Suspicious Patterns
- 4 separate files for death handling (DeathRecoveryManager, DeathHookIntegration, CorpsePreventionManager, SafeCorpseManager)
- Complex spawn chain: StartupSpawnOrchestrator → BotSpawnOrchestrator → BotSpawner → BotFactory → BotCharacterCreator
- Multiple session managers: BotSession, BotSessionManager, BotWorldSessionMgr

## Deliverables Needed
1. Architecture diagram (Mermaid)
2. Duplicate code report with consolidation plan
3. Thread safety analysis
4. Memory management analysis
5. State machine validation
6. Prioritized recommendations

## Reference
Full task details: .claude/prompts/ZENFLOW_SESSION_LIFECYCLE_ANALYSIS.md
```

---

## 📊 FILE STATISTICS

| Directory | Files | Est. LOC |
|-----------|-------|----------|
| Lifecycle/ | ~60 | ~15,000 |
| Session/ | ~25 | ~5,000 |
| **Total** | **~85** | **~20,000** |

### Largest Files (Priority)
| File | Lines | Concern |
|------|-------|---------|
| BotLifecycleManager.cpp | 860 | Why 2 managers? |
| BotLifecycleMgr.cpp | 826 | Duplicate? |
| BotSpawner.cpp | ~600 | Thread safety? |
| BotSession.cpp | ~500 | State machine? |
| DeathRecoveryManager.cpp | ~400 | Fragmented? |

---

## ⚡ QUICK DUPLICATE CHECK

```bash
# Files with suspiciously similar names:
BotLifecycleManager.cpp  vs  BotLifecycleMgr.cpp
BotSpawner.cpp           vs  BotSpawnOrchestrator.cpp
BotSession.cpp           vs  BotSessionManager.cpp
BotWorldSessionMgr.cpp   vs  BotSessionManager.cpp

# Same file in 2 directories:
Lifecycle/BotPerformanceMonitor.cpp
Session/BotPerformanceMonitor.cpp
```

---

## 🎯 EXPECTED OUTCOME

After ZenAI analysis, we should have:

1. **Clear understanding** of why duplicates exist
2. **Consolidation plan** to reduce 85 files to ~50
3. **Thread safety fixes** for spawning
4. **Memory leak fixes** for logout/death
5. **Simplified architecture** with clear ownership

---

## 📁 OUTPUT LOCATION

ZenAI should create reports in:
```
C:\Users\daimon\.zenflow\worktrees\session-lifecycle-analysis-XXXX\.zenflow\tasks\
├── ARCHITECTURE_ANALYSIS.md
├── DUPLICATE_REPORT.md
├── THREAD_SAFETY_REPORT.md
├── MEMORY_REPORT.md
├── STATE_MACHINE_ANALYSIS.md
└── RECOMMENDATIONS.md
```
