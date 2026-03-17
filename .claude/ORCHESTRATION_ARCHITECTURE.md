# Orchestration Architecture - Zenflow + Obsidian for TrinityCore

## System Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                     Zenflow Desktop (GUI)                        │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐          │
│  │   Project    │  │   Workflow   │  │     DAG      │          │
│  │  Dashboard   │  │   Designer   │  │  Visualizer  │          │
│  └──────────────┘  └──────────────┘  └──────────────┘          │
│                           │                                      │
│                           │ Orchestrates                         │
│                           ▼                                      │
└───────────────────────────────────────────────────────────────-─┘
                            │
        ┌───────────────────┼───────────────────┐
        │                   │                   │
        ▼                   ▼                   ▼
┌──────────────────┐ ┌──────────────────┐ ┌──────────────────┐
│ Claude Instance 1│ │ Claude Instance 2│ │ Claude Instance 3│
│  (Analysis)      │ │ (Implementation) │ │   (Review)       │
│                  │ │                  │ │                  │
│ Branch:          │ │ Branch:          │ │ Branch:          │
│ zenflow/agent-1/ │ │ zenflow/agent-2/ │ │ zenflow/agent-3/ │
└────────┬─────────┘ └────────┬─────────┘ └────────┬─────────┘
         │                    │                    │
         │    Shared Tasks    │    Shared Memory   │
         │  (via env var)     │   (via MCP)        │
         │                    │                    │
         └────────────────────┼────────────────────┘
                              │
                              ▼
              ┌───────────────────────────────┐
              │   CLAUDE_CODE_TASK_LIST_ID    │
              │  "trinitycore-playerbot-shared"│
              │                               │
              │   Task Coordination Layer     │
              │   • DAG dependencies          │
              │   • Status updates            │
              │   • Blocking relationships    │
              └───────────────┬───────────────┘
                              │
                              ▼
              ┌───────────────────────────────┐
              │     Obsidian Vault (MCP)      │
              │  C:\...\PlayerBotProjectMemory│
              │                               │
              │  ┌─────────────────────────┐  │
              │  │  Architecture/          │  │
              │  │  • System Overview.md   │  │
              │  │  • Component Docs       │  │
              │  └─────────────────────────┘  │
              │                               │
              │  ┌─────────────────────────┐  │
              │  │  Tasks/                 │  │
              │  │  • Active Instances.md  │  │
              │  │  • Task definitions     │  │
              │  └─────────────────────────┘  │
              │                               │
              │  ┌─────────────────────────┐  │
              │  │  Refactorings/          │  │
              │  │  • DI Cleanup.md        │  │
              │  │  • Sprint Plans         │  │
              │  └─────────────────────────┘  │
              │                               │
              │  ┌─────────────────────────┐  │
              │  │  Sessions/              │  │
              │  │  • Implementation notes │  │
              │  │  • Build reports        │  │
              │  │  • Handover docs        │  │
              │  └─────────────────────────┘  │
              │                               │
              │  ┌─────────────────────────┐  │
              │  │  Gotchas/               │  │
              │  │  • Git Workflow rules   │  │
              │  │  • Build system issues  │  │
              │  └─────────────────────────┘  │
              └───────────────┬───────────────┘
                              │
                              │ All agents read/write
                              │ Persistent memory
                              │ Graph visualization
                              │
                              ▼
              ┌───────────────────────────────┐
              │    TrinityCore Repository     │
              │  C:\TrinityBots\TrinityCore   │
              │                               │
              │  Branches:                    │
              │  • playerbot-dev (main)       │
              │  • zenflow/agent-1/sprint-2   │
              │  • zenflow/agent-2/sprint-2   │
              │  • zenflow/agent-3/review     │
              └───────────────────────────────┘
```

---

## Data Flow Diagram

### Workflow Execution Flow

```
User Action (Zenflow GUI)
    │
    │ Click "Run Workflow: DI Cleanup Sprint 2"
    │
    ▼
Zenflow Orchestrator
    │
    │ 1. Create workflow instance
    │ 2. Read DAG configuration
    │ 3. Identify first stage (Analysis)
    │
    ▼
Stage 1: Analysis Agent (Claude Opus 4.5)
    │
    │ Actions:
    │ • Create branch: zenflow/agent-1/sprint-2
    │ • Read Obsidian: "Refactorings/DI Cleanup.md"
    │ • Analyze dependencies
    │ • Write Obsidian: "Refactorings/Analysis Sprint 2.md"
    │ • Update task: Status = COMPLETE
    │
    ▼
Zenflow Orchestrator
    │
    │ Stage 1 complete ✅
    │ Check dependencies: Stage 2 unblocked
    │
    ▼
Stage 2: Specification Agent (Claude Opus 4.5)
    │
    │ Actions:
    │ • Create branch: zenflow/agent-2/sprint-2
    │ • Read Obsidian: "Refactorings/Analysis Sprint 2.md"
    │ • Generate technical spec
    │ • Write Obsidian: "Refactorings/Spec Sprint 2.md"
    │ • Update task: Status = COMPLETE
    │
    ▼
Zenflow Orchestrator
    │
    │ Stage 2 complete ✅
    │ Check dependencies: Stage 3 unblocked
    │
    ▼
Stage 3: Implementation Agent (Claude Sonnet 4.5)
    │
    │ Actions:
    │ • Create branch: zenflow/agent-3/sprint-2-impl
    │ • Read Obsidian: "Refactorings/Spec Sprint 2.md"
    │ • Check lock: .claude/locks/CMakeLists.txt.lock
    │ • Implement code changes
    │ • Update CMakeLists.txt
    │ • Commit changes
    │ • Write Obsidian: "Sessions/Implementation Sprint 2.md"
    │ • Update task: Status = COMPLETE
    │
    ▼
Zenflow Orchestrator
    │
    │ Stage 3 complete ✅
    │ Check dependencies: Stage 4 unblocked
    │
    ▼
Stage 4: Build Verification Agent (Claude Sonnet 4.5)
    │
    │ Actions:
    │ • Checkout impl branch
    │ • Run CMake configure
    │ • Build worldserver
    │ • Check for errors
    │ • Write Obsidian: "Sessions/Build Report Sprint 2.md"
    │ • Update task: Status = COMPLETE or FAILED
    │
    ▼
Zenflow Orchestrator
    │
    │ Stage 4 status check
    │ If FAILED: Alert user, stop workflow
    │ If COMPLETE: Continue to Stage 5
    │
    ▼
Stage 5: Review Agent (Claude Opus 4.5)
    │
    │ Actions:
    │ • Read all previous Obsidian notes
    │ • Review git diff
    │ • Check for issues:
    │   - Security vulnerabilities
    │   - Memory leaks
    │   - API misuse
    │ • Write Obsidian: "Sessions/Review Sprint 2.md"
    │ • Decision: APPROVE or REQUEST_CHANGES
    │ • Update task: Status = COMPLETE
    │
    ▼
Zenflow Orchestrator
    │
    │ Workflow complete ✅
    │ All stages passed
    │
    ▼
User Notification (Zenflow GUI)
    │
    │ "Sprint 2 workflow complete - Ready for merge"
    │
    ▼
User Reviews Obsidian Notes + Git Branches
    │
    │ Manually verify
    │ Merge when satisfied
    │
    ▼
Done ✅
```

---

## Communication Patterns

### Pattern 1: Task Coordination (via Shared Task List)

```
┌─────────────────┐         ┌─────────────────┐
│  Instance 1     │         │  Instance 2     │
│  (Terminal 1)   │         │  (Terminal 2)   │
└────────┬────────┘         └────────┬────────┘
         │                           │
         │ CLAUDE_CODE_TASK_LIST_ID  │
         │ = "trinitycore-..."       │
         │                           │
         └─────────┬─────────────────┘
                   │
                   ▼
         ┌──────────────────────┐
         │   Shared Task List   │
         │                      │
         │  1. [COMPLETE] Stage1│
         │  2. [IN_PROGRESS] S2 │◄─── Instance 1 updates
         │  3. [PENDING] Stage3 │
         │  4. [BLOCKED_BY: 3]  │◄─── Instance 2 sees this
         └──────────────────────┘
                   │
                   │ Both instances
                   │ read/write same list
                   │
         ┌─────────┴──────────┐
         │                    │
         ▼                    ▼
   Instance 1            Instance 2
   Knows: S2 active     Knows: Wait for S2
```

### Pattern 2: Memory Sharing (via Obsidian MCP)

```
┌─────────────────┐         ┌─────────────────┐
│  Agent 1        │         │  Agent 2        │
│  (Analysis)     │         │  (Impl)         │
└────────┬────────┘         └────────┬────────┘
         │                           │
         │ MCP: obsidian_write_note  │ MCP: obsidian_read_note
         │                           │
         └─────────┬─────────────────┘
                   │
                   ▼
         ┌──────────────────────────────┐
         │    Obsidian Vault            │
         │                              │
         │  "Analysis Sprint 2.md"      │
         │  ┌────────────────────────┐  │
         │  │ ## Dependencies Found  │  │
         │  │ - File1 depends on File2│  │◄── Agent 1 writes
         │  │ - CMakeLists needs update│ │
         │  └────────────────────────┘  │
         │                              │
         │                              │◄── Agent 2 reads
         │  Agent 2 sees this           │
         │  Uses it for implementation  │
         └──────────────────────────────┘
```

### Pattern 3: Conflict Prevention (via File Locks)

```
Agent 1 wants to edit CMakeLists.txt
    │
    │ 1. Check Obsidian for lock file
    ▼
Obsidian: ".claude/locks/CMakeLists.txt.lock"
    │
    ├─ EXISTS? ───────────────┐
    │                         │
    YES                       NO
    │                         │
    ▼                         ▼
Read lock info           Create lock file
    │                         │
    ├─ Expired?               Write:
    │  • locked_at            {
    NO   • release_after        "locked_by": "agent-1",
    │                           "locked_at": "2026-02-06T10:00:00Z",
    ▼                           "release_after": "2026-02-06T12:00:00Z"
⚠️ STOP                      }
Report conflict              │
Exit                         │
                             ▼
                        Edit CMakeLists.txt
                             │
                             ▼
                        Commit changes
                             │
                             ▼
                        Delete lock file
                             │
                             ▼
                        Done ✅
```

---

## State Management

### Workflow State Transitions

```
PENDING ──────────────────────────────────┐
   │                                      │
   │ User triggers workflow               │
   │                                      │
   ▼                                      │
IN_PROGRESS                               │
   │                                      │
   ├─ Stage 1 ─→ RUNNING ─→ COMPLETE ───┤
   │                                      │
   ├─ Stage 2 ─→ RUNNING ─→ COMPLETE ───┤
   │                                      │
   ├─ Stage 3 ─→ RUNNING ─→ COMPLETE ───┤
   │                                      │
   ├─ Stage 4 ─→ RUNNING ─→ FAILED ─────┼─→ WORKFLOW_FAILED
   │                          │           │        │
   │                          │           │        ▼
   │                          │           │   Alert user
   │                          │           │   Save logs to Obsidian
   │                          └─ Retry?   │   Stop execution
   │                              │       │
   │                              YES     │
   │                              │       │
   │                          RUNNING ───┤
   │                              │       │
   └──────────────────────────────┘       │
                                          │
All stages COMPLETE ──────────────────────┘
   │
   ▼
WORKFLOW_COMPLETE ✅
   │
   │ Notification to Zenflow GUI
   │ Update Obsidian summary
   │ Mark all tasks COMPLETE
   │
   ▼
Ready for manual review & merge
```

### Task State Transitions

```
PENDING
   │
   │ Agent claims task
   ▼
IN_PROGRESS
   │
   ├─ Work continues ───┐
   │                    │
   │ ◄──────────────────┘
   │
   ├─ Blocker encountered?
   │  YES ──→ BLOCKED
   │             │
   │             │ Blocker resolved
   │             ▼
   │          PENDING (restart)
   │
   ├─ Work completes
   │  YES ──→ COMPLETE ✅
   │
   └─ Abandoned?
      YES ──→ DELETED
```

---

## Security & Isolation

### Branch Isolation Strategy

```
Main Branch: playerbot-dev
    │
    │ Zenflow creates isolated branches per agent
    │
    ├─ zenflow/agent-1/sprint-2
    │   └─ Changes: Analysis only
    │
    ├─ zenflow/agent-2/sprint-2
    │   └─ Changes: Spec generation only
    │
    ├─ zenflow/agent-3/sprint-2-impl
    │   └─ Changes: Implementation only
    │
    └─ zenflow/agent-4/sprint-2-build
        └─ Changes: Build fixes only

    All isolated → Merge strategy:
    1. Review each branch independently
    2. Merge agent-1 first (analysis)
    3. Merge agent-2 second (spec)
    4. Merge agent-3 third (impl)
    5. Merge agent-4 last (fixes)

    No cross-contamination ✅
    Clear audit trail ✅
    Easy rollback ✅
```

### File Ownership Protection

```
┌────────────────────────────────────────┐
│        Obsidian: File Ownership        │
│                                        │
│  CMakeLists.txt                        │
│  ├─ Owner: Agent 3                     │
│  ├─ Lock expires: 2026-02-06 12:00    │
│  └─ Status: 🔒 LOCKED                  │
│                                        │
│  BattlegroundAI.cpp                    │
│  ├─ Owner: Agent 5                     │
│  ├─ Lock expires: Never (completed)   │
│  └─ Status: ✅ AVAILABLE                │
│                                        │
│  PlayerbotModule.cpp                   │
│  ├─ Owner: None                        │
│  ├─ Last modified: 2026-02-05         │
│  └─ Status: ✅ AVAILABLE                │
└────────────────────────────────────────┘
           │
           │ Agent 2 tries to edit CMakeLists.txt
           ▼
Check ownership → LOCKED by Agent 3
           │
           ▼
⚠️ CONFLICT DETECTED
   │
   ├─ Option 1: Wait for lock to expire
   ├─ Option 2: Request unlock from Agent 3
   └─ Option 3: Choose different file

   Conflict PREVENTED before it happens ✅
```

---

## Performance & Scalability

### Parallel Execution Capacity

```
Sequential (Before):
    Analysis → Spec → Impl → Build → Review
    │          │       │       │       │
    2h    +    1h   +  4h   +  1h  +   1h  = 9 hours total


Parallel (With Zenflow):
    Analysis (Agent 1) ──────────┐
         │                       │
         ▼                       │
    Spec (Agent 2) ──────────┐   │
         │                   │   │
         ▼                   │   │
    Impl (Agent 3) ───┐      │   │
         │            │      │   │
         ▼            ▼      ▼   ▼
    Build + Review in parallel after impl
         │
         ▼
    2h + 1h + 4h (max) + max(1h, 1h) = ~7 hours total

    Savings: 22% time reduction
    Benefits:
    • Different models per stage (cost optimization)
    • Each agent specializes
    • Failures isolated to stage
```

### Resource Usage

```
┌───────────────────────────────────────────┐
│         Resource Consumption              │
│                                           │
│  Zenflow Desktop:                         │
│  • CPU: ~5% (idle) / ~20% (active)        │
│  • RAM: ~200MB                            │
│  • Disk: ~500MB                           │
│                                           │
│  Obsidian:                                │
│  • CPU: ~2% (idle) / ~10% (indexing)      │
│  • RAM: ~100MB                            │
│  • Disk: ~50MB (vault) + 500MB (plugins) │
│                                           │
│  MCP Server:                              │
│  • CPU: ~1% (idle) / ~5% (requests)       │
│  • RAM: ~50MB                             │
│  • Disk: Negligible                       │
│                                           │
│  Claude Code (per instance):              │
│  • CPU: ~10-30% (during execution)        │
│  • RAM: ~500MB                            │
│  • Disk: Cache + logs (~1GB)             │
│                                           │
│  Total System Impact:                     │
│  • 3 Claude instances running: ~1.5GB RAM│
│  • Acceptable on 16GB+ system            │
└───────────────────────────────────────────┘
```

---

## Disaster Recovery

### What Happens When Things Go Wrong

```
Scenario: Agent 3 crashes during implementation

Before (No Orchestration):
    • All work lost
    • No record of progress
    • Must restart from scratch
    • Unknown what was attempted
    ❌ Hours wasted


After (With Zenflow + Obsidian):
    │
    │ Agent 3 crashes
    ▼
Zenflow detects failure
    │
    ├─ Saves logs to Obsidian: "Sessions/Crash Sprint 2.md"
    ├─ Marks task FAILED
    ├─ Preserves git branch: zenflow/agent-3/sprint-2-impl
    ├─ Updates Active Instances: Agent 3 = CRASHED
    └─ Alerts user

User can:
    │
    ├─ Read crash logs in Obsidian
    ├─ Review partial git branch
    ├─ Understand what was attempted
    ├─ Restart from last checkpoint
    └─ OR manually fix and continue

✅ Zero work lost
✅ Full audit trail
✅ Easy recovery
```

### Rollback Procedure

```
Problem: Sprint 2 implementation broke build

With Orchestration:
    │
    1. Check Obsidian: "Sessions/Build Report Sprint 2.md"
       └─ See exact error
    │
    2. Check git: zenflow/agent-3/sprint-2-impl
       └─ See all changes
    │
    3. Options:
       │
       ├─ A. Rollback entire sprint
       │     git reset --hard HEAD~N
       │     Delete Obsidian notes (archive)
       │
       ├─ B. Fix specific issue
       │     Read build logs
       │     Spawn new agent: "Fix build error X"
       │     Let agent fix specific problem
       │
       └─ C. Rollback to specific stage
             git checkout zenflow/agent-2/sprint-2  (spec was good)
             Re-run implementation stage with fixes

    All options are safe ✅
    All changes are tracked ✅
    Easy to find root cause ✅
```

---

## Cost Analysis

### Setup Costs (One-Time)

```
Time Investment:
• Obsidian setup:         30 minutes
• MCP server setup:       20 minutes
• Zenflow setup:          20 minutes
• Claude Code config:     15 minutes
• First workflow:         30 minutes
• Testing:               20 minutes
─────────────────────────────────────
Total:                   ~2-3 hours

Monetary Costs:
• Obsidian:              FREE
• Zenflow:               FREE (desktop version)
• MCP server:            FREE (open source)
• Claude Code:           INCLUDED (with Claude subscription)
─────────────────────────────────────
Total:                   $0 extra
```

### Ongoing Costs

```
Per Sprint/Workflow:
• Zenflow desktop:       FREE
• Obsidian storage:      ~50MB per sprint (negligible)
• MCP overhead:          Negligible CPU/RAM
• Claude API calls:      SAME (you're using Claude anyway)
                         Actually LESS due to better coordination
                         (no rework, no conflicts)

Time Savings Per Sprint:
• Setup overhead:        +5 minutes (create workflow)
• Execution:            -22% (parallel execution)
• Debugging:            -50% (clear logs, no conflicts)
• Rework:               -90% (spec-driven, no disasters)
─────────────────────────────────────
Net savings:            ~2-3 hours per sprint

ROI: POSITIVE after first real sprint ✅
```

---

## Comparison with Alternatives

| Approach | Setup Time | Coordination | Memory | GUI | Cost | Best For |
|----------|-----------|--------------|--------|-----|------|----------|
| **No Orchestration** | 0 min | ❌ Manual | ❌ None | ❌ | $0 | Solo, simple tasks |
| **Manual Git Branches** | 10 min | ⚠️ Manual | ❌ Docs only | ❌ | $0 | 2-3 instances max |
| **Built-in Task Lists** | 5 min | ✅ Automatic | ❌ None | ❌ | $0 | Basic coordination |
| **Zenflow + Obsidian** | 2-3 hrs | ✅ Automated | ✅ Full | ✅ | $0 | Complex projects |
| **Claude-Flow** | 1 hr | ✅ Swarms | ✅ Shared | ⚠️ Minimal | $0 | AI-first teams |
| **CrewAI** | 2 hrs | ✅ Crews | ✅ Shared | ✅ Visual | $0 | Multi-tool agents |
| **Custom Solution** | 40+ hrs | ✅ Your design | ✅ Your design | ✅ Your design | Dev time | Specific needs |

**Recommended:** Zenflow + Obsidian for TrinityCore project ✅

---

## Success Metrics

### How to Measure Success

```
Metric: Git Conflicts
Before: ~2-3 per week (manual coordination)
After:  ~0-1 per month (orchestrated)
Target: 90% reduction ✅

Metric: Rework Rate
Before: ~30% of code rewritten due to conflicts/misunderstandings
After:  ~5% (only genuine design changes)
Target: 80% reduction ✅

Metric: Context Switching Time
Before: 15-30 min to understand what others are doing
After:  2-5 min (read Obsidian notes)
Target: 75% reduction ✅

Metric: Disaster Recovery Time
Before: 2-4 hours (recreate lost work)
After:  10-15 min (restore from Obsidian + git)
Target: 90% reduction ✅

Metric: Onboarding New Instance
Before: 30-60 min (figure out state, conflicts)
After:  5 min (read Obsidian "Active Instances")
Target: 85% reduction ✅
```

---

**Document Version:** 1.0
**Last Updated:** 2026-02-06
**Architecture Stability:** Stable
**Recommended Review:** After completing 3 sprints
