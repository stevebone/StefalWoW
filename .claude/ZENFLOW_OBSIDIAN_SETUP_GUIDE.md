# Zenflow + Obsidian Setup Guide for TrinityCore Playerbot Project

**Purpose:** Orchestrate multiple Claude Code instances safely with GUI oversight and persistent project memory

**Target:** Prevent conflicts like the git stash disaster while enabling parallel development

**Timeline:** 2-3 hours initial setup, 30 minutes per new workflow

---

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Part 1: Obsidian Project Memory Setup](#part-1-obsidian-project-memory-setup)
3. [Part 2: Obsidian MCP Server Installation](#part-2-obsidian-mcp-server-installation)
4. [Part 3: Zenflow Desktop Installation](#part-3-zenflow-desktop-installation)
5. [Part 4: Claude Code Integration](#part-4-claude-code-integration)
6. [Part 5: TrinityCore Workflow Configuration](#part-5-trinitycore-workflow-configuration)
7. [Part 6: First Coordinated Task](#part-6-first-coordinated-task)
8. [Part 7: Advanced Patterns](#part-7-advanced-patterns)
9. [Troubleshooting](#troubleshooting)
10. [Daily Workflow](#daily-workflow)

---

## Prerequisites

### Required Software
- ✅ Windows 10/11 (you have this)
- ✅ Git (you have this)
- ✅ Node.js 18+ ([download](https://nodejs.org/))
- ✅ Claude Code CLI (you have this)
- ⚠️ Obsidian Desktop ([download](https://obsidian.md/download))
- ⚠️ Zenflow Desktop ([download](https://zencoder.ai/zenflow))

### Check Node.js Version
```bash
node --version
# Should show v18.x.x or higher
# If not, download from https://nodejs.org/
```

### Directory Structure (We'll Create)
```
C:\TrinityBots\
├── TrinityCore\                    # Your existing codebase
│   └── .claude\
│       ├── ZENFLOW_OBSIDIAN_SETUP_GUIDE.md (this file)
│       └── workflows\              # Zenflow workflow definitions
└── PlayerBotProjectMemory\         # NEW: Obsidian vault
    ├── .obsidian\                  # Obsidian config
    ├── Architecture\               # System documentation
    ├── Refactorings\               # Active work tracking
    ├── Gotchas\                    # Known issues
    └── Tasks\                      # Task coordination
```

---

## Part 1: Obsidian Project Memory Setup

### Step 1.1: Install Obsidian
1. Download from https://obsidian.md/download
2. Run installer
3. Launch Obsidian

### Step 1.2: Create Vault
1. Click "Create new vault"
2. Name: `TrinityCore-Playerbot-Memory`
3. Location: `C:\TrinityBots\PlayerBotProjectMemory`
4. Click "Create"

### Step 1.3: Install Required Obsidian Plugins

**A. Enable Community Plugins:**
1. Settings (gear icon) → Community plugins
2. Turn off "Restricted mode"
3. Click "Browse"

**B. Install "Local REST API" Plugin:**
1. Search: "Local REST API"
2. Install by @coddingtonbear
3. Enable the plugin
4. Go to plugin settings
5. Copy the API key (you'll need this later)
6. Enable "HTTPS" (optional but recommended)

**C. Install "Dataview" Plugin (Optional but Recommended):**
1. Search: "Dataview"
2. Install and enable
3. Allows querying notes programmatically

### Step 1.4: Create Initial Vault Structure

Create these folders in Obsidian (right-click → New folder):
- `Architecture/`
- `Refactorings/`
- `Gotchas/`
- `Tasks/`
- `Sessions/`
- `Decisions/`

### Step 1.5: Create Core Documentation Files

**File: `Architecture/System Overview.md`**
```markdown
# TrinityCore Playerbot System Overview

## Project Info
- **Repository:** C:\TrinityBots\TrinityCore
- **Branch Strategy:**
  - main: playerbot-dev
  - refactoring: separate branches per major work
- **Build System:** CMake 3.24+ with MSVC
- **Size:** ~636,000 LOC (Playerbot module)

## Critical Systems
- [[AI System]] - BotAI, ClassAI, Coordination
- [[Combat System]] - CombatManager, ThreatManager
- [[Movement System]] - MovementArbiter, Pathfinding
- [[PvP System]] - BattlegroundAI, Arena, LFG
- [[Quest System]] - UnifiedQuestManager
- [[Event System]] - EventBus architecture (recently refactored)

## Known Active Work
- [[Refactorings/DI Cleanup]] - Multi-day effort, in progress
- [[Refactorings/EventBus Consolidation]] - Removing individual *EventBus.h files

## See Also
- [[Gotchas/Build System Gotchas]]
- [[Gotchas/Git Workflow Gotchas]]
```

**File: `Gotchas/Git Workflow Gotchas.md`**
```markdown
# Git Workflow Gotchas - TrinityCore Playerbot

## ⚠️ CRITICAL: Never Use Git Stash with Multiple Active Instances

**Incident:** 2026-02-06 - Stash Disaster
- One Claude instance ran `git stash push` on 22 files
- Those files contained multi-day refactoring work from ANOTHER instance
- Stash captured partial work and reset files
- Required emergency recovery branch

**Rule:** NEVER `git stash` when multiple instances are working
- Use `git commit` to WIP branch instead
- Check `[[Tasks/Active Instances]]` before ANY git operation

## Branch Isolation Required

Each Claude Code instance MUST work on separate branch:
- Instance 1: `refactoring-di-cleanup`
- Instance 2: `bugfix-tok-combat`
- Instance 3: `feature-xyz`

**Merge Protocol:**
1. Smaller changes merge first
2. Larger refactorings merge last
3. Use Zenflow to coordinate merge timing

## File Ownership Tracking

Before modifying files, check:
- [[Tasks/Active Instances]] - Who's working on what
- [[Refactorings/File Ownership]] - Current assignments

## CMakeLists.txt Is Critical

**Status:** Currently being refactored
**Issues:**
- DI interfaces removed (Interfaces/ directory deleted)
- EventBus.h files consolidated
- 13 obsolete references removed on 2026-02-06

**Rule:** Only ONE instance should modify CMakeLists.txt at a time
- Create lock file: `.claude/locks/CMakeLists.txt.lock`
- Check lock before editing

## See Also
- [[Architecture/Build System]]
- [[Refactorings/DI Cleanup]]
```

**File: `Refactorings/DI Cleanup.md`**
```markdown
# DI Cleanup Refactoring - Active Work

**Status:** 🔄 IN PROGRESS (Started: 2026-02-03)
**Owner:** Instance 1 (Unknown terminal/session)
**Branch:** playerbot-dev (or unknown)
**Priority:** 🔴 CRITICAL - Do Not Interrupt

## Scope
- Remove DI (Dependency Injection) interface files
- Consolidate event bus system
- Update CMakeLists.txt references
- Migrate to unified event architecture

## Files Modified (Known)
- `src/modules/Playerbot/CMakeLists.txt` ⚠️ LOCKED
- `src/modules/Playerbot/Core/DI/Interfaces/` (directory DELETED)
- Multiple *EventBus.h files (being consolidated)
- ~34 files in AI/Coordination/ area

## Progress Tracking
- [x] Sprint 1: Core DI removal
- [ ] Sprint 2: Raid coordination files (IN PROGRESS)
- [ ] Sprint 3: Arena coordination files
- [ ] Sprint 4: EventBus consolidation
- [ ] Sprint 5: Build verification and cleanup

## Known Issues
- CMakeLists.txt had 13 obsolete file references (fixed 2026-02-06)
- Recovery branch created: `recovery-refactoring-work`

## Coordination Notes
- **Other instances MUST NOT touch these files until marked DONE**
- Check this file before starting any new work
- Update progress here after each sprint

## Contact
- Update `[[Tasks/Active Instances]]` if you're the owner
```

**File: `Tasks/Active Instances.md`**
```markdown
# Active Claude Code Instances

**Last Updated:** 2026-02-06 06:00 UTC

## Instance 1 - Primary Refactoring (ACTIVE)
- **Terminal:** Unknown (check for running worldserver/cmake processes)
- **Branch:** `playerbot-dev` or unknown
- **Task:** DI Cleanup + EventBus Consolidation
- **Status:** 🔄 Sprint 2/5 - Raid coordination
- **ETA:** Unknown
- **Files Locked:**
  - `src/modules/Playerbot/CMakeLists.txt`
  - `src/modules/Playerbot/AI/Coordination/` (all files)
- **Last Activity:** Unknown

**⚠️ DO NOT INTERRUPT THIS INSTANCE**

## Instance 2 - Standby (YOU)
- **Terminal:** Current session
- **Branch:** `recovery-refactoring-work` (archive only)
- **Task:** Standing by, waiting for Instance 1 completion
- **Status:** ⏸️ STANDBY
- **Available For:** New isolated work on separate branch only

## Adding New Instance

When starting new instance, add entry here:
```markdown
## Instance X - [Task Name]
- **Terminal:** [Terminal identifier]
- **Branch:** `feature-xyz` or `bugfix-xyz`
- **Task:** [One-line description]
- **Status:** 🔄 Active / ⏸️ Standby / ✅ Complete
- **Files Locked:** [List files or directories]
- **Last Activity:** [Timestamp]
```

Then commit this file so all instances see it.

## Instance Coordination Protocol

Before starting work:
1. Read this file
2. Check for conflicts
3. Add your entry
4. Commit and push
5. Start work on YOUR branch only

Before git operations:
1. Re-read this file (might have updated)
2. Check `git status` for unexpected modifications
3. If you see files you didn't modify → STOP
4. Check if another instance is active

## See Also
- [[Gotchas/Git Workflow Gotchas]]
- [[Refactorings/DI Cleanup]]
```

**File: `Tasks/Task Template.md`**
```markdown
# Task: [Task Name]

**ID:** TASK-[YYYY-MM-DD]-[NN]
**Created:** [Date]
**Assigned To:** [Instance X]
**Branch:** `branch-name`
**Priority:** 🔴 Critical / 🟡 High / 🟢 Normal / 🔵 Low
**Status:** 📋 Planned / 🔄 In Progress / ✅ Complete / ❌ Blocked

## Description
[What needs to be done]

## Acceptance Criteria
- [ ] Criterion 1
- [ ] Criterion 2
- [ ] Build passes
- [ ] Tests pass (if applicable)

## Dependencies
**Blocks:** [[TASK-YYYY-MM-DD-NN]]
**Blocked By:** [[TASK-YYYY-MM-DD-NN]]

## Files Involved
- `path/to/file1.cpp`
- `path/to/file2.h`

## Implementation Notes
[Technical details, approach, gotchas]

## Testing Plan
- [ ] Local build verification
- [ ] Manual testing: [describe]
- [ ] Integration testing: [describe]

## Related Documentation
- [[Architecture/Relevant System]]
- [[Gotchas/Related Gotcha]]

## Activity Log
- [YYYY-MM-DD HH:MM] - Started implementation
- [YYYY-MM-DD HH:MM] - Completed feature X
- [YYYY-MM-DD HH:MM] - Hit blocker: [description]
```

### Step 1.6: Enable Graph View

1. Open Obsidian Graph View (icon in left sidebar or `Ctrl+G`)
2. Settings → Appearance → Graph
3. Enable:
   - "Show attachments"
   - "Show existing only"
   - "Links" (bold)
4. Color code by folder (optional)

Now you have a visual map of your project knowledge!

---

## Part 2: Obsidian MCP Server Installation

### Step 2.1: Clone MCP Server Repository

```bash
cd C:\TrinityBots
git clone https://github.com/cyanheads/obsidian-mcp-server.git
cd obsidian-mcp-server
```

### Step 2.2: Install Dependencies

```bash
npm install
```

### Step 2.3: Build the Server

```bash
npm run build
```

### Step 2.4: Configure MCP Server

Create config file: `C:\TrinityBots\obsidian-mcp-server\config.json`

```json
{
  "vaultPath": "C:\\TrinityBots\\PlayerBotProjectMemory",
  "obsidianRestApiKey": "YOUR_API_KEY_FROM_OBSIDIAN_PLUGIN",
  "obsidianRestApiUrl": "https://127.0.0.1:27124",
  "logLevel": "info"
}
```

**Replace `YOUR_API_KEY_FROM_OBSIDIAN_PLUGIN` with the key from Step 1.3.B**

### Step 2.5: Test MCP Server

```bash
cd C:\TrinityBots\obsidian-mcp-server
node dist/index.js
```

You should see:
```
[INFO] Obsidian MCP Server started
[INFO] Connected to vault: C:\TrinityBots\PlayerBotProjectMemory
[INFO] Listening for MCP connections...
```

**Press Ctrl+C to stop** (we'll run it via Claude Code config next)

---

## Part 3: Zenflow Desktop Installation

### Step 3.1: Download Zenflow

1. Visit https://zencoder.ai/zenflow
2. Click "Download for Windows"
3. Run the installer (`ZenflowSetup.exe`)
4. Follow installation wizard

### Step 3.2: Launch Zenflow

1. Open Zenflow Desktop
2. Sign in or create account (required for sync)
3. You'll see the main dashboard

### Step 3.3: Create TrinityCore Project

1. Click "New Project"
2. Project Name: `TrinityCore Playerbot`
3. Project Path: `C:\TrinityBots\TrinityCore`
4. Project Type: `C++ / CMake`
5. AI Providers: Check "Anthropic Claude"
6. Click "Create"

### Step 3.4: Configure AI Providers

1. Settings → AI Providers
2. Click "Add Provider"
3. Select "Anthropic"
4. Enter your Anthropic API key
   - Or use `ANTHROPIC_API_KEY` environment variable
5. Select models:
   - ✅ Claude Opus 4.5 (for complex reasoning)
   - ✅ Claude Sonnet 4.5 (for implementation)
   - ✅ Claude Haiku (for quick tasks)
6. Save

### Step 3.5: Connect to Repository

1. Project Settings → Version Control
2. Git Repository: `C:\TrinityBots\TrinityCore`
3. Default Branch: `playerbot-dev`
4. Enable "Branch per agent" ✅
5. Branch Prefix: `zenflow/agent-{agent-id}/`
6. Save

---

## Part 4: Claude Code Integration

### Step 4.1: Configure Claude Code MCP

Edit your Claude Code MCP config file:

**Location:** `~/.config/claude-code/mcp.json` (Windows: `%APPDATA%\Roaming\claude-code\mcp.json`)

```json
{
  "mcpServers": {
    "obsidian-memory": {
      "command": "node",
      "args": [
        "C:\\TrinityBots\\obsidian-mcp-server\\dist\\index.js"
      ],
      "env": {
        "OBSIDIAN_VAULT_PATH": "C:\\TrinityBots\\PlayerBotProjectMemory",
        "OBSIDIAN_API_KEY": "YOUR_API_KEY_HERE",
        "OBSIDIAN_API_URL": "https://127.0.0.1:27124"
      }
    },
    "trinitycore": {
      "command": "...",
      "// NOTE": "Keep your existing TrinityCore MCP server config"
    }
  }
}
```

### Step 4.2: Verify MCP Connection

```bash
# In new terminal
claude-code

# In Claude conversation:
> Can you list the available MCP tools?
```

You should see Obsidian tools:
- `obsidian_read_note`
- `obsidian_write_note`
- `obsidian_search_notes`
- `obsidian_list_notes`
- `obsidian_create_note`

### Step 4.3: Test Obsidian Integration

```bash
# In Claude Code session:
> Read the note "Architecture/System Overview" from my Obsidian vault
```

Claude should retrieve and display the content.

### Step 4.4: Configure Task List Sharing

Add to your shell profile (`.bashrc` or `.bash_profile` on Windows Git Bash):

```bash
export CLAUDE_CODE_TASK_LIST_ID="trinitycore-playerbot-shared"
```

Or set in Windows Environment Variables:
1. Windows Key → "Environment Variables"
2. User variables → New
3. Name: `CLAUDE_CODE_TASK_LIST_ID`
4. Value: `trinitycore-playerbot-shared`
5. OK

**Restart all terminals** after setting this.

### Step 4.5: Enable Agent Teams (Optional)

For multi-agent coordination within a single Claude instance:

```bash
export CLAUDE_CODE_EXPERIMENTAL_AGENT_TEAMS=1
```

Or Windows Environment Variable:
- Name: `CLAUDE_CODE_EXPERIMENTAL_AGENT_TEAMS`
- Value: `1`

---

## Part 5: TrinityCore Workflow Configuration

### Step 5.1: Create "DI Refactoring" Workflow in Zenflow

1. Open Zenflow Desktop
2. Projects → TrinityCore Playerbot
3. Click "New Workflow"
4. Name: `DI Cleanup Refactoring`
5. Click "Use Template" → "Spec-Driven Development"

### Step 5.2: Configure Workflow Stages

Zenflow workflows use DAG (Directed Acyclic Graph). Configure these stages:

**Stage 1: Analysis**
- Name: `Analyze Dependencies`
- Agent Model: `Claude Opus 4.5`
- Instructions:
  ```
  Read the Obsidian note "Refactorings/DI Cleanup" to understand current state.
  Analyze dependencies in the target files.
  Identify all files that need modification.
  Write analysis to Obsidian: "Refactorings/DI Cleanup Analysis Sprint N.md"
  Create task list in Obsidian with file-by-file breakdown.
  ```
- Input: User-provided sprint number
- Output: Analysis markdown file path

**Stage 2: Specification**
- Name: `Create Technical Spec`
- Agent Model: `Claude Opus 4.5`
- Depends On: `Analyze Dependencies` ✅
- Instructions:
  ```
  Read the analysis from Stage 1.
  Create detailed technical specification:
  - Files to modify
  - Changes required
  - Migration strategy
  - Rollback plan
  - Testing approach
  Write spec to Obsidian: "Refactorings/DI Cleanup Spec Sprint N.md"
  ```
- Input: Analysis file path (from Stage 1)
- Output: Spec markdown file path

**Stage 3: Implementation**
- Name: `Implement Changes`
- Agent Model: `Claude Sonnet 4.5`
- Depends On: `Create Technical Spec` ✅
- Instructions:
  ```
  Read the spec from Stage 2.
  Implement changes following the spec exactly.
  Update CMakeLists.txt if needed (check for lock first!).
  Write implementation notes to Obsidian: "Sessions/Sprint N Implementation.md"
  Log any deviations from spec with justification.
  ```
- Input: Spec file path (from Stage 2)
- Output: Commit hash

**Stage 4: Build Verification**
- Name: `Verify Build`
- Agent Model: `Claude Sonnet 4.5`
- Depends On: `Implement Changes` ✅
- Instructions:
  ```
  Run CMake configuration.
  Build worldserver target.
  Check for errors.
  If errors: analyze and fix OR rollback.
  Write build report to Obsidian: "Sessions/Sprint N Build Report.md"
  ```
- Input: Commit hash (from Stage 3)
- Output: Build status (PASS/FAIL)

**Stage 5: Review & Merge**
- Name: `Code Review`
- Agent Model: `Claude Opus 4.5` (different from implementation)
- Depends On: `Verify Build` ✅
- Instructions:
  ```
  Review implemented changes.
  Compare against spec.
  Check for:
  - Security issues
  - Memory leaks
  - API misuse
  - Breaking changes
  Write review to Obsidian: "Sessions/Sprint N Review.md"
  Approve for merge OR request changes.
  ```
- Input: Build status + commit hash
- Output: Review decision (APPROVE/REQUEST_CHANGES)

### Step 5.3: Configure Workflow Triggers

1. Workflow Settings → Triggers
2. Add Trigger: "Manual"
   - Type: User-initiated
   - Prompt: "Enter sprint number (e.g., '2' for Sprint 2)"
3. Add Trigger: "On Git Push" (optional)
   - Branch pattern: `refactoring-*`
   - Run: `Verify Build` stage only

### Step 5.4: Set Up Agent Isolation

1. Workflow Settings → Agent Configuration
2. ✅ Enable "Isolated Branches"
   - Branch prefix: `zenflow/di-cleanup-sprint-{N}/agent-{stage}`
3. ✅ Enable "Parallel Execution"
   - Max parallel agents: 2 (since stages are sequential, this applies to retries)
4. ✅ Enable "Shared Memory"
   - Memory provider: "Obsidian MCP"
   - Vault path: `C:\TrinityBots\PlayerBotProjectMemory`

### Step 5.5: Configure Notifications

1. Workflow Settings → Notifications
2. Add notification: "Stage Complete"
   - Trigger: Any stage completes
   - Action: Update Obsidian note `Tasks/Active Instances.md`
3. Add notification: "Workflow Failed"
   - Trigger: Any stage fails
   - Action: Create Obsidian note `Sessions/Failure Sprint N.md`
4. Add notification: "Build Failed"
   - Trigger: Build Verification fails
   - Action: Alert user + log to Obsidian

---

## Part 6: First Coordinated Task

Let's run a simple test task to verify everything works.

### Step 6.1: Create Test Task in Obsidian

File: `Tasks/TEST-2026-02-06-01 - Verify Setup.md`

```markdown
# Task: Verify Zenflow + Obsidian Setup

**ID:** TEST-2026-02-06-01
**Created:** 2026-02-06
**Assigned To:** Test Agent
**Branch:** `test-zenflow-setup`
**Priority:** 🟢 Normal
**Status:** 📋 Planned

## Description
Verify that Zenflow and Obsidian integration works correctly by:
1. Reading this task from Obsidian
2. Writing a test note to Obsidian
3. Checking task coordination

## Acceptance Criteria
- [ ] Agent can read tasks from Obsidian
- [ ] Agent can write notes to Obsidian
- [ ] Zenflow shows agent progress
- [ ] No conflicts with other instances

## Implementation Notes
This is a test task. No actual code changes required.

## Activity Log
- 2026-02-06 - Task created
```

### Step 6.2: Start Test Workflow in Zenflow

1. Open Zenflow Desktop
2. Go to TrinityCore Playerbot project
3. Click "Run Workflow" → `DI Cleanup Refactoring`
4. Enter sprint: `TEST`
5. Click "Start"

### Step 6.3: Watch in Real-Time

**In Zenflow:**
- You'll see the DAG visualization
- Each stage lights up as it runs
- Click on stages to see agent output

**In Obsidian:**
- Open Graph View (`Ctrl+G`)
- Watch new notes appear in `Sessions/` folder
- Open `Tasks/Active Instances.md` to see updates

### Step 6.4: Verify Obsidian Integration

After workflow completes:

1. Check Obsidian for new files:
   - `Refactorings/DI Cleanup Analysis Sprint TEST.md`
   - `Refactorings/DI Cleanup Spec Sprint TEST.md`
   - `Sessions/Sprint TEST Implementation.md`
   - `Sessions/Sprint TEST Build Report.md`
   - `Sessions/Sprint TEST Review.md`

2. Check Git:
   ```bash
   cd C:\TrinityBots\TrinityCore
   git branch | grep zenflow
   # Should show: zenflow/di-cleanup-sprint-TEST/agent-*
   ```

3. Check Claude Code task list:
   ```bash
   claude-code
   > /tasks
   # Should show tasks from the workflow
   ```

### Step 6.5: Cleanup Test

```bash
git branch -D zenflow/di-cleanup-sprint-TEST/*
# Delete test branches
```

Delete test notes in Obsidian:
- Right-click `Sessions/Sprint TEST*` files
- Delete

---

## Part 7: Advanced Patterns

### Pattern 1: File Locking

**Problem:** Two agents try to edit same file

**Solution:** Lock file pattern in Obsidian

File: `.claude/locks/CMakeLists.txt.lock`

```json
{
  "file": "src/modules/Playerbot/CMakeLists.txt",
  "locked_by": "zenflow/agent-3",
  "locked_at": "2026-02-06T10:30:00Z",
  "reason": "Adding 50 new files during Sprint 2",
  "release_after": "2026-02-06T12:00:00Z"
}
```

**Agent Instruction (add to all workflow stages):**
```
Before modifying any file, check Obsidian for lock file:
- Look for ".claude/locks/{filename}.lock"
- If exists and not expired: STOP and report conflict
- If you're locking a file: Create lock file in Obsidian
- After done: Delete lock file
```

### Pattern 2: Session Handover

**Problem:** One agent finishes, need to handover to next agent

**Solution:** Structured session notes

File: `Sessions/Session Handover Template.md`

```markdown
# Session Handover - [From Agent] → [To Agent]

**Handover Date:** [Date]
**From:** Agent ID / Instance
**To:** Agent ID / Instance

## Work Completed
- ✅ Task 1
- ✅ Task 2
- ⚠️ Task 3 (partial - see notes)

## Current State
- Branch: `branch-name`
- Last Commit: `abc123def`
- Build Status: ✅ PASSING / ❌ FAILING
- Tests: ✅ PASSING / ❌ FAILING / ⚠️ NOT RUN

## Work In Progress
- File: `src/path/to/file.cpp`
  - Status: 50% complete
  - Next Step: Implement function X
  - Blocker: Need decision on approach Y

## Next Agent Should
1. Read [[Architecture/Relevant Docs]]
2. Review `src/path/to/file.cpp` changes
3. Continue implementation of feature X
4. Run build verification

## Known Issues
- Issue 1: Description and workaround
- Issue 2: Description and workaround

## Questions for User
- Question 1
- Question 2

## Files Modified
- `src/file1.cpp` - ✅ Complete
- `src/file2.h` - ⚠️ In progress
- `src/file3.cpp` - 📋 Not started
```

### Pattern 3: Emergency Stop

**Problem:** Need to stop all agents immediately

**Solution:** Emergency stop file

File: `Tasks/EMERGENCY_STOP.md`

```markdown
# 🚨 EMERGENCY STOP 🚨

**Issued:** [Date Time]
**Reason:** [Critical issue description]

## All Agents Must:
1. STOP current work immediately
2. Commit WIP to current branch
3. Write handover note to Sessions/
4. Mark tasks as BLOCKED
5. Update `Tasks/Active Instances.md` status to ⏸️ STANDBY

## Issue Description
[Detailed description of why emergency stop was issued]

## Resolution Required
- [ ] Action item 1
- [ ] Action item 2

## Resume Instructions
Delete this file when issue is resolved and agents can resume.
```

**Agent Instruction (add to ALL workflow stages):**
```
Before starting each major step:
1. Check Obsidian for "Tasks/EMERGENCY_STOP.md"
2. If exists: Stop, commit WIP, write handover, exit
3. If not exists: Continue normal operation
```

### Pattern 4: Dependency Tracking

**Problem:** Task depends on another task's completion

**Solution:** Task dependency links

In each task note, use Obsidian links:

```markdown
## Dependencies
**Blocks:**
- [[TASK-2026-02-06-03]] - Cannot start until this is done
- [[TASK-2026-02-06-05]]

**Blocked By:**
- [[TASK-2026-02-06-01]] - Waiting for this to complete
```

Then use Dataview query to check:

```markdown
# Tasks Blocked By This Task

​```dataview
LIST
FROM "Tasks"
WHERE contains(BlockedBy, [[]])
​```
```

### Pattern 5: Cross-Repository Coordination

**Problem:** TrinityCore + another repo (e.g., database changes)

**Solution:** Multi-project Obsidian vault

Extend vault structure:
```
PlayerBotProjectMemory/
├── Projects/
│   ├── TrinityCore/
│   │   ├── Architecture/
│   │   ├── Tasks/
│   │   └── ...
│   └── Database/
│       ├── Schema Changes/
│       ├── Migration Scripts/
│       └── ...
└── Cross-Project/
    └── Integration Tasks/
```

Link tasks across projects:
```markdown
## Related Tasks
- TrinityCore: [[Projects/TrinityCore/Tasks/TASK-X]]
- Database: [[Projects/Database/Tasks/TASK-Y]]
```

---

## Troubleshooting

### Issue: MCP Server Won't Start

**Symptoms:** Claude Code can't connect to Obsidian

**Check:**
1. Obsidian Local REST API plugin is enabled
2. API key is correct in MCP config
3. Vault path is correct
4. No firewall blocking localhost:27124

**Fix:**
```bash
# Test manually
cd C:\TrinityBots\obsidian-mcp-server
node dist/index.js
# Should start without errors
```

### Issue: Zenflow Can't Find Git Repo

**Symptoms:** "Repository not found" error

**Fix:**
1. Zenflow Settings → Projects → TrinityCore
2. Click "Re-scan Repository"
3. Verify path: `C:\TrinityBots\TrinityCore`
4. Check that `.git` folder exists

### Issue: Task List Not Shared Between Instances

**Symptoms:** Each Claude instance has different tasks

**Check:**
```bash
echo $CLAUDE_CODE_TASK_LIST_ID
# Should output: trinitycore-playerbot-shared
```

**Fix:**
- Set environment variable properly
- Restart ALL terminals
- Both instances must have same value

### Issue: Obsidian Graph View Doesn't Show Links

**Symptoms:** Notes exist but no connections visible

**Fix:**
- Use `[[Note Name]]` syntax for links, not `[Note Name](path)`
- Refresh graph view: `Ctrl+G` (close and reopen)
- Check Settings → Files & Links → "Detect all file extensions" ✅

### Issue: Zenflow Workflow Hangs

**Symptoms:** Agent stage runs forever

**Check Zenflow Logs:**
1. Zenflow → Workflow → Click stage
2. View logs in bottom panel
3. Look for errors

**Common Causes:**
- Agent waiting for user input (but shouldn't be)
- Build command hangs (increase timeout)
- MCP server crashed (check Obsidian)

**Fix:**
- Cancel stage
- Check logs
- Fix issue
- Restart workflow from failed stage

### Issue: Git Conflicts Between Agents

**Symptoms:** Merge conflicts when agents try to merge

**Prevention:**
- Use Zenflow's "Branch per agent" feature
- Check `Tasks/Active Instances.md` before starting
- Lock files with `.claude/locks/` pattern

**Recovery:**
1. Stop all agents
2. Identify conflict source
3. Manually resolve in Git
4. Update `Tasks/Active Instances.md`
5. Resume agents on clean state

---

## Daily Workflow

### Morning Startup

**1. Check Obsidian Status:**
```bash
# Open Obsidian
# Read: Tasks/Active Instances.md
# Read: Tasks/EMERGENCY_STOP.md (shouldn't exist)
# Read: Refactorings/DI Cleanup.md (current state)
```

**2. Check Git Status:**
```bash
cd C:\TrinityBots\TrinityCore
git status
git branch | grep zenflow
# Should be clean or only YOUR branches
```

**3. Open Zenflow:**
- Launch Zenflow Desktop
- Check running workflows
- Review yesterday's progress

### Starting New Task

**1. Create Task in Obsidian:**
- Use `Tasks/Task Template.md`
- Fill in all sections
- Link dependencies
- Assign to agent/instance

**2. Check for Conflicts:**
- Read `Tasks/Active Instances.md`
- Verify no file ownership conflicts
- Check for locks in `.claude/locks/`

**3. Create Branch:**
```bash
git checkout playerbot-dev
git pull origin playerbot-dev
git checkout -b task-specific-branch-name
```

**4. Update Active Instances:**
- Edit `Tasks/Active Instances.md`
- Add your instance entry
- Commit and push

**5. Start Zenflow Workflow:**
- Select appropriate workflow
- Provide task details
- Monitor progress

### During Work

**Every Hour:**
- Check Obsidian Graph View for new notes
- Review `Tasks/Active Instances.md` for conflicts
- Commit WIP to your branch

**When Stuck:**
- Write to `Sessions/Blocker [Date].md`
- Update task status to ❌ BLOCKED
- Notify in `Tasks/Active Instances.md`

### End of Day

**1. Commit All Work:**
```bash
git add .
git commit -m "WIP: [Description] - End of day 2026-02-06"
git push origin your-branch-name
```

**2. Write Session Handover:**
- Create `Sessions/Handover [Date].md`
- Document current state
- List next steps

**3. Update Obsidian:**
- Update task statuses
- Update `Tasks/Active Instances.md`
- Write to `Decisions/` if made architectural decisions

**4. Close Zenflow:**
- Review workflow progress
- Pause running workflows if incomplete
- Save logs

### Weekly Review

**Every Sunday:**

**1. Obsidian Maintenance:**
- Review all open tasks
- Close completed tasks
- Archive old sessions
- Update architecture docs

**2. Git Cleanup:**
```bash
# Delete merged branches
git branch --merged | grep -v playerbot-dev | xargs git branch -d

# Delete abandoned zenflow branches
git branch | grep "zenflow/" | xargs git branch -D
```

**3. Zenflow Review:**
- Export workflow metrics
- Review agent performance
- Update workflow configurations

---

## Quick Reference

### Essential Obsidian Notes to Check Daily
1. `Tasks/Active Instances.md` - Who's working on what
2. `Tasks/EMERGENCY_STOP.md` - Should NOT exist
3. `Refactorings/DI Cleanup.md` - Current refactoring status
4. `Gotchas/Git Workflow Gotchas.md` - Rules reminder

### Essential Git Commands
```bash
# Check for conflicts
git status

# List Zenflow branches
git branch | grep zenflow

# Safe stash alternative
git commit -m "WIP: Saving state before switch"

# Check what others are doing
git fetch --all
git branch -r | grep zenflow
```

### Essential Zenflow Operations
- **Start Workflow:** Projects → TrinityCore → Run Workflow
- **Check Status:** Dashboard → Active Workflows
- **View Logs:** Workflow → Stage → Logs Panel
- **Emergency Stop:** Workflow → Stop (red button)

### Essential Claude Code Commands
```bash
# List tasks
/tasks

# Create task
/task-create "Task name" "Description"

# Update task
/task-update <id> --status completed

# Show MCP tools
> List available MCP tools
```

### Emergency Contacts
- **Zenflow Support:** https://zencoder.ai/support
- **MCP Server Issues:** https://github.com/cyanheads/obsidian-mcp-server/issues
- **Claude Code Docs:** https://code.claude.com/docs

---

## Next Steps

After completing this setup:

1. ✅ Test with small task (Part 6)
2. ✅ Run one full refactoring sprint using Zenflow
3. ✅ Refine workflows based on experience
4. ✅ Add custom workflow stages as needed
5. ✅ Integrate with CI/CD (optional)
6. ✅ Set up automated backups of Obsidian vault

## Success Metrics

You'll know the setup is working when:
- ✅ Multiple Claude instances coordinate without conflicts
- ✅ All work is documented in Obsidian automatically
- ✅ Zenflow GUI shows real-time agent progress
- ✅ Git history is clean (no emergency recovery branches)
- ✅ Build passes after each sprint
- ✅ You can resume work after days/weeks from Obsidian notes

---

**Document Version:** 1.0
**Last Updated:** 2026-02-06
**Maintained By:** Project Team
**Review Schedule:** Weekly
