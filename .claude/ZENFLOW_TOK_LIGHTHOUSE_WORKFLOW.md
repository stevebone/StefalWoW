# Zenflow Workflow: TOK Lighthouse Battleground

**Complete workflow configuration for Temple of Kotmogu - 100% correct & fully dynamic**

---

## Workflow Metadata

**Workflow Name:** `TOK Lighthouse Battleground`

**Description:** Make Temple of Kotmogu the first 100% correct, fully dynamic battleground. This will serve as the template/blueprint for all 13 other battlegrounds.

**Branch Prefix:** `lighthouse-bg/tok/`

**Project:** TrinityCore Playerbot

**Agent:** Claude Code (for all automated stages)

---

## Stage 1: Research TOK Data

**Stage Name:** `Research TOK Data (DBC/DB2/SQL)`

**Agent:** Claude Code

**Depends On:** None (first stage)

**Instructions:**
```
Read from Obsidian: "Refactorings/TOK Lighthouse BG.md"

Research and verify:
1. Orb GameObject entries (should be 212091-212094)
   - Query: SELECT * FROM gameobject WHERE id IN (212091,212092,212093,212094)
   - Verify spawn positions, display IDs, states

2. Orb aura IDs (should be 121175-121178)
   - Check spell data for each aura
   - Verify buff mechanics (1x, 2x, 3x, 4x multipliers)

3. Victory conditions
   - Confirm 1600 points to win
   - Score calculation formula
   - Center control multiplier (5x)

4. Map data
   - Zone ID for Temple of Kotmogu
   - Center area coordinates
   - Orb spawn positions (exact coordinates)

5. Current implementation analysis
   - Read: src/modules/Playerbot/PvP/BattlegroundAI.cpp (TOK functions)
   - Read: src/modules/Playerbot/AI/Coordination/Battleground/Scripts/Domination/TempleOfKotmoguScript.cpp
   - Identify all bugs and issues

Document findings:
- Write to Obsidian: "Sessions/TOK Sprint 1 - Data Research.md"
- Include: All GameObject data, aura data, current bugs found
- List: What needs to be fixed vs what's already correct
```

---

## Stage 2: Create Perfect Specification

**Stage Name:** `Create TOK Perfect Spec`

**Agent:** Claude Code

**Depends On:** Stage 1 ✓

**Instructions:**
```
Read from Obsidian:
- "Sessions/TOK Sprint 1 - Data Research.md" (Stage 1 output)
- "Refactorings/TOK Lighthouse BG.md" (requirements)

Create detailed technical specification covering:

1. GameObject Interaction Fix
   - Exact code changes needed for orb pickup
   - Correct GameObject::Use() parameters
   - State checks required
   - Error handling approach

2. Combat Engagement Implementation
   - Where to add Attack() calls
   - Target selection logic
   - Combat state management
   - Integration with existing combat system

3. Dynamic Strategy State Machine
   - State definitions (e.g., AGGRESSIVE, DEFENSIVE, BALANCED)
   - Transition triggers (score difference, orb count, time)
   - Actions per state
   - Role assignment per state

4. Center Control Logic
   - Detection of center area
   - Movement to center
   - 5x multiplier awareness
   - Risk/reward calculation

5. Edge Case Handling
   - Bot death with orb (orb drop behavior)
   - Orb respawn timing (30 seconds)
   - Multiple bots targeting same orb
   - Player disconnect/AFK
   - Uneven team sizes

6. Data Validation
   - Confirm all GameObject entries
   - Confirm all aura IDs
   - Confirm spawn positions
   - Confirm victory conditions

7. Testing Criteria
   - Success metrics (>95% orb pickup, 100% combat engagement)
   - Performance requirements (<0.1% CPU per bot)
   - Zero crashes requirement

Write specification to Obsidian: "Refactorings/TOK Lighthouse Spec.md"

Specification must be:
- Complete (no TODOs or "to be determined")
- Precise (exact line numbers, exact code changes)
- Testable (clear acceptance criteria)
- Reviewable (clear rationale for each decision)
```

---

## Stage 3: Fix GameObject & Combat

**Stage Name:** `Fix Orb Pickup & Combat Engagement`

**Agent:** Claude Code

**Depends On:** Stage 2 ✓

**Instructions:**
```
Read specification: "Refactorings/TOK Lighthouse Spec.md"

Implementation tasks:

1. Fix GameObject Orb Pickup
   File: src/modules/Playerbot/PvP/BattlegroundAI.cpp

   - Locate PickupOrb() function (around line 1750)
   - Fix GameObject search to use correct entries:
     * 212091 (Orange Orb)
     * 212092 (Purple Orb)
     * 212093 (Green Orb)
     * 212094 (Blue Orb)
   - Add proper GameObject state checks (GAMEOBJECT_STATE_READY)
   - Add distance validation (must be within USE_RANGE)
   - Add proper error handling
   - Add detailed logging for debugging

2. Merge Combat Engagement Fix

   - Check if fix exists in recovery-refactoring-work branch
   - If yes: Cherry-pick combat engagement commits
   - If no: Implement from scratch:
     * HuntEnemyOrbCarrier() - Add player->Attack(enemyCarrier, true)
     * DefendOrbCarrier() - Add player->Attack(enemy, true)
   - Ensure Attack() is called AFTER SetSelection()
   - Add combat state validation (IsAlive, IsHostileTo)
   - Add detailed logging

3. Add Center Control Awareness

   - Define center area coordinates
   - Add IsInCenterArea() helper function
   - Modify movement logic to prioritize center when carrying orb
   - Add 5x multiplier awareness to decision making

4. Code Quality

   - Remove all TODOs
   - Add comprehensive error handling
   - Add null pointer checks
   - Add bounds checking
   - Follow TrinityCore coding style

5. Testing Preparation

   - Add detailed logging statements for:
     * Orb pickup attempts (success/failure with reason)
     * Combat engagements (target, result)
     * Center control decisions
     * State transitions

Write implementation notes to Obsidian: "Sessions/TOK Sprint 2 - GameObject & Combat Fix.md"

Include:
- Exact changes made (file:line references)
- Rationale for each change
- Any deviations from spec (with justification)
- Known limitations or edge cases not yet handled
```

---

## Stage 4: Implement Dynamic Strategy

**Stage Name:** `Implement Dynamic Tactics & Strategy`

**Agent:** Claude Code

**Depends On:** Stage 3 ✓

**Instructions:**
```
Read from Obsidian:
- "Refactorings/TOK Lighthouse Spec.md" (strategy design)
- "Sessions/TOK Sprint 2 - GameObject & Combat Fix.md" (current state)

Implement dynamic strategy system:

1. Create Strategy State Machine
   File: src/modules/Playerbot/AI/Coordination/Battleground/Scripts/Domination/TempleOfKotmoguScript.cpp

   States:
   - AGGRESSIVE: Ahead in score, hunt enemy carriers
   - DEFENSIVE: Behind in score, protect our carriers
   - BALANCED: Even score, standard play
   - DESPERATION: Far behind + time running out
   - PRESERVATION: Far ahead + time running out

   Transitions:
   - Based on score difference (thresholds: 200, 400 points)
   - Based on orb distribution (who has more orbs)
   - Based on time remaining
   - Based on player count advantage

2. Implement Score-Based Tactics

   - Add GetCurrentScore() for both teams
   - Add GetScoreDifference() helper
   - Add GetOrbDistribution() (count orbs per team)
   - Implement tactics per state:
     * AGGRESSIVE: Focus on enemy carriers, center control
     * DEFENSIVE: Protect carriers, avoid risky plays
     * BALANCED: Standard role distribution
     * DESPERATION: All-in aggressive, ignore defense
     * PRESERVATION: Avoid combat, run down clock

3. Implement Orb Distribution Analysis

   - Track which team holds which orbs
   - Count total orbs per team
   - Identify priority targets (enemy with multiple orbs)
   - Adjust strategy based on orb count

4. Implement Role Reassignment
   File: src/modules/Playerbot/AI/Coordination/Battleground/BGRoleManager.cpp

   - Dynamic role changes based on strategy state
   - Role distribution per state:
     * AGGRESSIVE: More hunters, fewer defenders
     * DEFENSIVE: More defenders/escorts, fewer hunters
     * BALANCED: Even distribution
   - Smooth role transitions (don't abandon mid-action)

5. Add Strategy Logging

   - Log state transitions (old state → new state + reason)
   - Log strategy decisions (why choosing target X)
   - Log role reassignments
   - Performance metrics (state duration, effectiveness)

6. Integration with BGCoordinator
   File: src/modules/Playerbot/AI/Coordination/Battleground/BattlegroundCoordinator.cpp

   - Connect strategy state to coordinator
   - Broadcast strategy changes to all bots
   - Coordinate multi-bot actions (synchronized attacks)

Write to Obsidian: "Sessions/TOK Sprint 3 - Dynamic Strategy.md"

Include:
- State machine diagram (text-based)
- Transition logic explanation
- Role distribution tables
- Example scenarios showing strategy adaptation
```

---

## Stage 5A: Build Verification & Test Prep

**Stage Name:** `Build & Prepare Tests`

**Agent:** Claude Code

**Depends On:** Stage 4 ✓

**Instructions:**
```
Read from Obsidian: "Refactorings/TOK Testing Plan.md"

Tasks:

1. Build Verification

   - Navigate to build directory
   - Run CMake configuration:
     cmake --build . --config RelWithDebInfo --target worldserver -- /maxcpucount:4
   - Check for compilation errors
   - Check for warnings (should be minimal)
   - Verify binary created successfully
   - Check binary size (compare to previous build)

2. Static Code Analysis

   - Check all modified files for:
     * Null pointer dereferences
     * Uninitialized variables
     * Memory leaks (missing deletes)
     * Buffer overflows
     * Race conditions (missing locks)
     * Exception safety
   - Verify all TODOs removed
   - Verify all error paths handled
   - Check for code smells (duplicated code, long functions)

3. Generate bg.log Configuration

   - Create snippet for worldserver.conf:
     Logger.BG.TOK=6,Console Server File
     Logger.BG.TOK.File=bg
     Logger.BG.TOK.TimeStamp=1
     Logger.playerbots.bg=6,Console Server File
     Logger.playerbots.bg.File=bg

4. Create Manual Test Checklist

   Generate detailed test instructions in Obsidian: "Sessions/TOK Test Instructions.md"

   Include:
   - Server startup instructions
   - How to add bg.log config
   - Exact test scenarios:
     * Test 1: Orange orb pickup (bot approaches, uses, confirms aura)
     * Test 2: Purple orb pickup
     * Test 3: Green orb pickup
     * Test 4: Blue orb pickup
     * Test 5: Combat engagement (bot attacks enemy carrier)
     * Test 6: Center control (bot moves to center with orb)
     * Test 7: Strategy switch (force score difference, observe behavior)
     * Test 8: Bot death with orb (verify orb drops)
     * Test 9: Orb respawn (verify 30 second timer)
     * Test 10: Full game (10v10, complete to victory condition)

   - Metrics to track:
     * Orb pickup attempts vs successes (target: >95%)
     * Combat engagements vs failures to attack (target: 100%)
     * Crashes (target: ZERO)
     * Strategy transitions observed
     * Victory condition correctness

   - Log collection instructions:
     * Where to find logs/bg.log
     * What log excerpts to copy
     * How to save to Obsidian

5. Create Test Results Template

   Create file in Obsidian: "Sessions/TOK Manual Test Results.md"

   Template with sections:
   - Test Environment (build hash, date, duration)
   - Orb Pickup Results (table with attempts/successes per orb)
   - Combat Engagement Results
   - Strategy Transition Observations
   - Crashes / Errors
   - Performance Notes
   - Log Excerpts
   - Overall Assessment

Write to Obsidian:
- "Sessions/TOK Build Report.md" - Build results, warnings, binary info
- "Sessions/TOK Code Analysis.md" - Static analysis findings
- "Sessions/TOK Test Instructions.md" - Complete test checklist (ALREADY DESCRIBED ABOVE)

Output should be:
- Build report: ✅ PASS or ❌ FAIL (if fail, stop workflow here)
- Code analysis: List of issues found (if critical, stop workflow)
- Test instructions: Complete, clear, ready for human tester
```

---

## Stage 5B: MANUAL TESTING PAUSE

**Stage Name:** `⏸️ PAUSE: User Manual Testing`

**Agent:** None (Human Task)

**Depends On:** Stage 5A ✓

**Type:** Manual Pause Point

**Instructions:**
```
⏸️⏸️⏸️ WORKFLOW PAUSES HERE ⏸️⏸️⏸️

This is a manual testing phase. The workflow will resume after user completes testing.

USER TASKS:

1. Read Test Instructions
   - Open Obsidian: "Sessions/TOK Test Instructions.md"
   - Review all test scenarios
   - Understand success criteria

2. Configure Server
   - Add bg.log configuration to worldserver.conf (snippet provided in Stage 5A)
   - Ensure Playerbot module is enabled
   - Start worldserver

3. Execute Tests
   - Follow each test scenario in order
   - Track metrics for each test
   - Note any unexpected behavior
   - Collect screenshots if issues occur

4. Collect Logs
   - After testing, copy logs/bg.log
   - Extract relevant sections (orb pickup events, combat events, errors)
   - Save to a file for analysis

5. Fill Results Template
   - Open Obsidian: "Sessions/TOK Manual Test Results.md"
   - Fill in all sections:
     * Test environment details
     * Results for each test scenario
     * Metrics (success rates)
     * Crashes (if any)
     * Log excerpts (paste relevant logs)
     * Overall observations

6. Resume Workflow
   - After completing all tests and documenting results
   - Resume workflow to Stage 5C
   - Stage 5C will analyze your results

ESTIMATED TIME: 30-60 minutes

SUCCESS CRITERIA TO PROCEED:
- ✅ All test scenarios executed
- ✅ Results documented in Obsidian
- ✅ Logs collected and excerpts saved
- ✅ Ready for automated analysis

NOTE: If major issues found during testing:
- Document them clearly
- Stage 5C will identify them
- Workflow may loop back to fix issues
```

---

## Stage 5C: Log Analysis & Validation

**Stage Name:** `Analyze Test Results & Validate`

**Agent:** Claude Code

**Depends On:** Stage 5B ✓

**Instructions:**
```
Read from Obsidian:
- "Sessions/TOK Manual Test Results.md" (user's test data)
- "Refactorings/TOK Testing Plan.md" (success criteria)
- Any attached log files referenced in test results

Analysis tasks:

1. Parse Test Results

   - Extract metrics from user's results:
     * Orb pickup success rate (per orb and overall)
     * Combat engagement success rate
     * Crash count
     * Strategy transitions observed
     * Victory condition correctness
   - Compare against targets:
     * Orb pickup: >95% required
     * Combat: 100% required
     * Crashes: ZERO required

2. Log File Analysis

   - Parse log excerpts for:
     * Error messages (ERROR, FATAL levels)
     * Warning messages (relevant to TOK)
     * Exception traces
     * Performance issues (long delays, timeouts)
   - Search for specific events:
     * "Orb pickup attempt" → success/failure reasons
     * "Combat engagement" → target acquired, attack started
     * "Strategy transition" → old state → new state
     * GameObject interaction failures
     * Null pointer accesses
     * Memory allocation failures

3. Performance Analysis

   - Check for:
     * Excessive CPU usage indicators
     * Memory leaks (increasing allocations)
     * Slow operations (>100ms for simple actions)
     * Thread contention / deadlocks
   - Verify performance requirements met:
     * <0.1% CPU per bot
     * <10MB memory per bot
     * <50ms response time for decisions

4. Edge Case Verification

   - Verify each edge case was tested:
     * Bot death with orb → orb dropped correctly
     * Orb respawn timing → 30 seconds confirmed
     * Multiple bots same orb → conflict resolution
     * Disconnect/AFK → system handled gracefully
   - Check if edge cases passed

5. Decision Matrix

   Based on analysis, make decision:

   ✅ PASS - Proceed to Stage 6 if ALL true:
   - Orb pickup ≥95%
   - Combat engagement = 100%
   - Crashes = 0
   - No critical errors in logs
   - Performance acceptable
   - All edge cases handled

   ⚠️ NEEDS FIXES - Create fix tasks if:
   - Orb pickup <95% (identify failure reasons)
   - Combat engagement <100% (identify causes)
   - Crashes = 1-2 (not critical, but needs fixing)
   - Some errors in logs (non-critical)
   - Performance issues (but not blocking)

   ❌ FAIL - Recommend rollback if:
   - Crashes ≥3 (critical stability issue)
   - Critical errors in logs (data corruption, memory corruption)
   - Orb pickup <50% (fundamental design flaw)
   - Performance unacceptable (>1% CPU per bot, >50MB memory)

6. Fix Task Generation (if NEEDS FIXES)

   - For each issue found, create task in Obsidian:
     * "Tasks/TOK-FIX-[NN] - [Issue Description].md"
     * Include: Root cause, proposed fix, affected files
   - Update "Refactorings/TOK Lighthouse BG.md" with fix status

Write comprehensive analysis to Obsidian: "Sessions/TOK Log Analysis Report.md"

Include:
- Executive summary (PASS/NEEDS FIXES/FAIL + rationale)
- Detailed metrics breakdown
- Log analysis findings (errors, warnings, patterns)
- Performance assessment
- Edge case verification results
- If NEEDS FIXES: List of issues + fix tasks created
- If FAIL: Recommendations for next steps (rollback vs redesign)
- If PASS: Confirmation to proceed to blueprint stage

WORKFLOW DECISION:
- If PASS: Continue to Stage 6
- If NEEDS FIXES: Pause workflow, alert user, wait for fixes
- If FAIL: Stop workflow, alert user, recommend rollback
```

---

## Stage 6: Create Transfer Blueprint

**Stage Name:** `Create Transfer Blueprint for All BGs`

**Agent:** Claude Code

**Depends On:** Stage 5C ✓ (PASS status required)

**Instructions:**
```
Read from Obsidian:
- "Refactorings/TOK Lighthouse BG.md" (original requirements)
- "Refactorings/TOK Lighthouse Spec.md" (design decisions)
- All session notes (Sprint 1-3, test results, analysis)
- "Sessions/TOK Log Analysis Report.md" (validation confirmation)

Create comprehensive blueprint for transferring TOK patterns to all 13 other battlegrounds.

Blueprint document: "Architecture/Lighthouse BG Blueprint.md"

Structure:

1. Executive Summary
   - What makes TOK the lighthouse
   - Success metrics achieved
   - Key patterns to transfer
   - Estimated effort to apply to other BGs

2. Data Accuracy Pattern
   - How we researched TOK data (DBC/DB2/SQL queries)
   - GameObject verification process
   - Aura/spell verification process
   - Victory condition verification
   - Template queries for other BGs
   - Checklist for data validation

3. GameObject Interaction Pattern
   - The correct way to interact with BG objects
   - GameObject::Use() best practices
   - State checking requirements
   - Distance validation
   - Error handling pattern
   - Code template for object interaction

4. Combat Engagement Pattern
   - SetSelection() + Attack() sequence
   - Target validation (IsAlive, IsHostileTo)
   - Combat state management
   - Integration with existing combat system
   - Code template for combat engagement

5. Dynamic Strategy Pattern
   - State machine architecture
   - Transition logic design
   - How to define BG-specific states
   - How to define transition triggers
   - Role assignment integration
   - Code template for strategy system

6. Center Control / Objective Pattern
   - How to define objective areas
   - Priority calculation
   - Risk/reward analysis
   - Movement coordination
   - Code template for objective control

7. Edge Case Handling Pattern
   - Common edge cases across all BGs
   - BG-specific edge cases
   - Testing methodology
   - Code patterns for edge case handling

8. Coordination Pattern
   - BGCoordinator integration
   - Role manager usage
   - Multi-bot synchronization
   - Communication patterns
   - Code template for coordination

9. Logging & Debugging Pattern
   - What to log and why
   - Log levels for different events
   - Performance logging
   - Debugging tools used
   - Log configuration template

10. Testing Pattern
    - Build verification process
    - Static code analysis checklist
    - Manual testing scenarios template
    - Log analysis methodology
    - Success criteria definition
    - Metrics tracking template

11. Performance Pattern
    - CPU usage targets
    - Memory usage targets
    - Response time requirements
    - Performance profiling tools
    - Optimization techniques used

12. Code Quality Pattern
    - TrinityCore coding standards applied
    - Error handling requirements
    - Null safety practices
    - Resource management (RAII)
    - Code review checklist

13. Transfer Process
    - Step-by-step guide to apply patterns to new BG
    - Customization points (what's BG-specific vs generic)
    - Estimated time per BG
    - Dependency order (which BGs to do first)
    - Risk mitigation (testing, validation)

14. BG Priority Matrix
    - List all 13 other battlegrounds
    - Categorize by type:
      * CTF: Warsong Gulch, Twin Peaks
      * Domination: Arathi Basin, Battle for Gilneas, Eye of the Storm
      * Resource Race: Silvershard Mines, Deepwind Gorge
      * Siege: Alterac Valley, Isle of Conquest, Strand of the Ancients
      * Epic: Ashran
    - Recommend order of implementation
    - Complexity assessment per BG

15. Success Metrics for Transfer
    - How to measure if pattern was applied correctly
    - Validation checklist per BG
    - Testing requirements per BG
    - Sign-off criteria

16. Lessons Learned
    - What worked well in TOK development
    - What challenges were faced
    - What would be done differently
    - Tips for applying to other BGs

17. Code Appendix
    - Complete code templates (copy-paste ready)
    - Configuration snippets
    - SQL queries for data verification
    - Testing scripts

Write blueprint to Obsidian: "Architecture/Lighthouse BG Blueprint.md"

Also create quick reference:
- "Architecture/BG Transfer Checklist.md" - Quick checklist for applying patterns

Update project documentation:
- "Refactorings/TOK Lighthouse BG.md" - Mark as ✅ COMPLETE
- "Tasks/Active Instances.md" - Update status

Blueprint should be:
- Complete (covers all aspects)
- Actionable (anyone can follow it)
- Template-based (minimal customization per BG)
- Validated (based on proven TOK implementation)
- Maintainable (easy to update if patterns improve)

FINAL OUTPUT:
- Blueprint document (50-100 pages equivalent)
- Transfer checklist (2-3 pages)
- Updated project status
- Ready for application to next BG
```

---

## Workflow Settings

**If Zenflow allows these configurations:**

- ✅ **Branch Strategy:** Create branch per stage
  - Format: `lighthouse-bg/tok/stage-{N}-{stage-name}`
  - Merge after validation

- ✅ **Shared Memory:** Obsidian Vault
  - Path: `C:\TrinityBots\PlayerBotProjectMemory\TrinityCore-Playerbot-Memory`
  - All stages read/write to Obsidian for coordination

- ✅ **Parallelization:** Sequential (not parallel)
  - Max parallel: 1
  - Stages must complete in order

- ✅ **Auto-commit:** After each stage
  - Commit message format: `[Zenflow] Stage {N}: {stage-name} - {brief summary}`

- ✅ **Failure Handling:**
  - On build failure (Stage 5A): Stop workflow, alert user
  - On test failure (Stage 5C): Pause workflow, create fix tasks
  - On any critical error: Stop workflow, alert user

- ✅ **Notifications:**
  - Stage complete: Update Obsidian "Tasks/Active Instances.md"
  - Workflow paused: Alert user with reason
  - Workflow complete: Summary report to Obsidian

---

## Expected Timeline

- **Stage 1 (Research):** ~30-45 minutes
- **Stage 2 (Spec):** ~45-60 minutes
- **Stage 3 (GameObject/Combat):** ~60-90 minutes
- **Stage 4 (Dynamic Strategy):** ~90-120 minutes
- **Stage 5A (Build & Prep):** ~15-30 minutes
- **Stage 5B (Manual Testing):** ~30-60 minutes (USER)
- **Stage 5C (Analysis):** ~20-30 minutes
- **Stage 6 (Blueprint):** ~60-90 minutes

**Total Automated Time:** ~6-8 hours
**Total Manual Time:** ~30-60 minutes (your testing)
**Total Workflow Time:** ~7-9 hours

---

## Workflow Success Criteria

To consider the workflow successful:

- ✅ All stages complete without critical errors
- ✅ Build passes (Stage 5A)
- ✅ Manual tests pass (Stage 5B/5C):
  - Orb pickup ≥95%
  - Combat engagement = 100%
  - Zero crashes
- ✅ Blueprint created (Stage 6)
- ✅ Code merged to main branch
- ✅ Documentation complete in Obsidian

**Result:** Temple of Kotmogu is production-ready and serves as the blueprint for all other battlegrounds.

---

**End of Workflow Definition**
