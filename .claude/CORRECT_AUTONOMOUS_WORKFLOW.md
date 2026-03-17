# Correct Autonomous Workflow - Human Reviews FIX, Not PR

## ✅ Correct Design (User Feedback Applied)

**Key Insight:** Human should review the **generated fix** before it's deployed, not after deployment in a PR.

---

## 🔄 Correct Workflow

```
┌─────────────────────────────────────────────────────────────────┐
│ PHASE 1: AUTONOMOUS DETECTION & ANALYSIS                       │
└─────────────────────────────────────────────────────────────────┘

Crash Detected
      ↓ (30 seconds)
Python Monitor Detects Request
      ↓ (instant)
Creates Marker for Claude
      ↓ (30 seconds)
Claude Detects Marker
      ↓ (5-10 minutes)
Claude Analyzes Crash (Trinity MCP + Serena MCP)
      ↓ (instant)
Claude Generates Fix
      ↓ (instant)
Writes Response JSON (fix details)

┌─────────────────────────────────────────────────────────────────┐
│ PHASE 2: HUMAN REVIEW (APPROVAL GATE)                          │
└─────────────────────────────────────────────────────────────────┘

Python Creates REVIEW_{id}.txt
      ↓
Shows Fix Details to Human:
  - Crash analysis summary
  - Root cause
  - Proposed fix strategy
  - Code changes
  - Files modified
      ↓
Human Reviews Fix Quality
      ↓
Human Makes Decision:
  ├── APPROVE: Creates approval_{id}.json with status="approved"
  └── REJECT: Creates approval_{id}.json with status="rejected"

┌─────────────────────────────────────────────────────────────────┐
│ PHASE 3: AUTONOMOUS DEPLOYMENT (IF APPROVED)                   │
└─────────────────────────────────────────────────────────────────┘

Python Detects Approval File (status="approved")
      ↓ (instant)
Applies Fix to Source Files
      ↓ (5-15 minutes)
Compiles worldserver
      ↓ (optional)
Runs Unit Tests
      ↓ (instant)
Creates Git Commit
      ↓ (instant)
Creates GitHub PR
      ↓ (instant)
Marks as Deployed
      ↓
[DONE - Ready for Production]
```

---

## 📊 Comparison

### ❌ Wrong Approach (Previous Design)
```
Generate Fix → Apply → Compile → PR → [HUMAN REVIEWS PR] → Merge
```
**Problem:** By the time human reviews, fix is already compiled and committed. Wasteful if fix is bad.

### ✅ Correct Approach (Current Design)
```
Generate Fix → [HUMAN REVIEWS FIX] → Apply → Compile → PR → Merge
```
**Benefit:** Human reviews fix BEFORE any compilation or deployment. Saves time if fix is rejected.

---

## 🎯 Key Differences

### Phase Boundaries

#### Phase 1: Autonomous (No Human)
- Crash detection
- CDB analysis
- Trinity MCP research
- Serena MCP codebase analysis
- Root cause identification
- Fix strategy design
- Code generation
- Response JSON writing

**Output:** `response_{id}.json` with complete fix details

#### Phase 2: Human Review (APPROVAL GATE)
- Read fix description
- Review code changes
- Assess fix quality
- Make decision: APPROVE or REJECT

**Input:** `REVIEW_{id}.txt` (human-readable summary)
**Output:** `approval_{id}.json` (approval decision)

#### Phase 3: Autonomous (If Approved)
- Apply fix to filesystem
- Compile worldserver
- Run tests
- Create git commit
- Create GitHub PR
- Deploy

**Input:** `approval_{id}.json` with `status="approved"`
**Output:** Deployed fix + GitHub PR

---

## 📁 File Structure

```
.claude/crash_analysis_queue/
├── requests/
│   └── request_{id}.json          # Created by crash detector
├── responses/
│   └── response_{id}.json         # Created by Claude (fix details)
├── approvals/
│   ├── REVIEW_{id}.txt            # Created by Python (human-readable)
│   └── approval_{id}.json         # Created by HUMAN (decision)
└── auto_process/
    └── process_{id}.json          # Trigger for Claude
```

---

## 🧪 Example Approval Request

### File: `REVIEW_abc123.txt`
```
╔══════════════════════════════════════════════════════════════════════
║ CRASH FIX READY FOR HUMAN REVIEW
╚══════════════════════════════════════════════════════════════════════

REQUEST ID: abc123
CRASH ID: 273f92f0f16d
GENERATED: 2025-10-31T12:00:00

══════════════════════════════════════════════════════════════════════
CRASH ANALYSIS SUMMARY
══════════════════════════════════════════════════════════════════════

Exception: Access Violation (C0000005)
Location: EventDispatcher.cpp:245
Function: EventDispatcher::UnsubscribeAll

Root Cause:
Virtual method GetManagerId() called on partially destructed object
during UnsubscribeAll cleanup. The manager's vtable is already invalid
when we try to log the manager ID.

══════════════════════════════════════════════════════════════════════
PROPOSED FIX
══════════════════════════════════════════════════════════════════════

Strategy: Early State Capture with Exception Safety

Files Modified:
  - src/modules/Playerbot/Core/Events/EventDispatcher.cpp

Fix Description:
Capture manager ID string BEFORE calling unsubscribe operations.
Use try-catch block for exception safety. Fall back to "<unknown>"
if GetManagerId() throws during object destruction.

Code Changes:
- Add early ID capture: std::string managerId = manager->GetManagerId();
- Wrap in try-catch for safety
- Use captured string in logging (not direct manager call)

══════════════════════════════════════════════════════════════════════
REVIEW INSTRUCTIONS
══════════════════════════════════════════════════════════════════════

1. Review the full response JSON at:
   .claude/crash_analysis_queue/responses/response_abc123.json

2. Assess fix quality:
   - Is the root cause correct?
   - Is the fix appropriate?
   - Are there any edge cases missed?
   - Does it follow project conventions?

3. Make your decision:

   TO APPROVE (deploy the fix):
   ----------------------------------------------------------------------
   Create file: .claude/crash_analysis_queue/approvals/approval_abc123.json

   Content:
   {
     "request_id": "abc123",
     "status": "approved",
     "approved_by": "YOUR_NAME",
     "approved_at": "2025-10-31T12:05:00",
     "comments": "Fix looks good, early capture prevents use-after-free"
   }

   TO REJECT (do not deploy):
   ----------------------------------------------------------------------
   Create file: .claude/crash_analysis_queue/approvals/approval_abc123.json

   Content:
   {
     "request_id": "abc123",
     "status": "rejected",
     "rejected_by": "YOUR_NAME",
     "rejected_at": "2025-10-31T12:05:00",
     "reason": "Fix is incomplete, needs to handle X case"
   }

══════════════════════════════════════════════════════════════════════

Once you create the approval file, the system will automatically:
- Apply the fix to source code (if approved)
- Compile worldserver
- Run tests
- Create git commit
- Create GitHub PR
- Deploy to production

HUMAN DECISION REQUIRED - System waiting for your approval...
```

---

## 💡 Why This Is Better

### Time Efficiency
**If fix is good:**
- Human reviews fix in 2-3 minutes
- Approves immediately
- Deployment proceeds automatically
- **Total: ~35 minutes (30min autonomous + 3min review + 2min deployment)**

**If fix is bad:**
- Human reviews fix in 2-3 minutes
- Rejects immediately
- NO compilation wasted
- NO commit created
- NO PR created
- **Total: ~13 minutes (10min analysis + 3min review) - saves 20+ minutes**

### Quality Gate Placement
- **Too Early:** Reviewing crash before analysis (wasteful, Claude can analyze)
- **✅ Just Right:** Reviewing fix before deployment (optimal approval point)
- **Too Late:** Reviewing PR after deployment (wasteful if fix is bad)

### Resource Optimization
- **Compilation:** Only happens for approved fixes (~15 minutes saved if rejected)
- **Git History:** Clean, no rejected fixes committed
- **PR Clutter:** No PRs for rejected fixes

---

## 🚀 How to Use

### Start the Correct System
```bash
cd C:\TrinityBots\TrinityCore
python .claude\scripts\autonomous_crash_monitor_with_approval.py
```

### When You See a Review Request
1. **Check logs:**
   ```bash
   tail -f .claude/logs/autonomous_monitor_approval.log
   ```

2. **You'll see:**
   ```
   🔔 NEW FIX READY FOR HUMAN REVIEW
   ═══════════════════════════════════════
   Request ID: abc123
   Crash ID: 273f92f0f16d

   📝 Created approval request: REVIEW_abc123.txt
   ⏳ WAITING FOR HUMAN REVIEW OF FIX

      Review file: .claude/crash_analysis_queue/approvals/REVIEW_abc123.txt
      Response: .claude/crash_analysis_queue/responses/response_abc123.json

      Create approval file to proceed:
      .claude/crash_analysis_queue/approvals/approval_abc123.json
   ```

3. **Read the review file:**
   ```bash
   cat .claude/crash_analysis_queue/approvals/REVIEW_abc123.txt
   ```

4. **Read full response (optional):**
   ```bash
   cat .claude/crash_analysis_queue/responses/response_abc123.json | jq
   ```

5. **Make decision:**

   **To approve:**
   ```bash
   cat > .claude/crash_analysis_queue/approvals/approval_abc123.json << 'EOF'
   {
     "request_id": "abc123",
     "status": "approved",
     "approved_by": "YourName",
     "approved_at": "2025-10-31T12:00:00",
     "comments": "Fix looks good"
   }
   EOF
   ```

   **To reject:**
   ```bash
   cat > .claude/crash_analysis_queue/approvals/approval_abc123.json << 'EOF'
   {
     "request_id": "abc123",
     "status": "rejected",
     "rejected_by": "YourName",
     "rejected_at": "2025-10-31T12:00:00",
     "reason": "Fix is incomplete, needs to handle edge case X"
   }
   EOF
   ```

6. **Wait 30 seconds** - Python will detect approval and deploy automatically (if approved)

---

## 📊 Expected Timelines

### Full Success Path
```
00:00 - Crash occurs
00:30 - Detection complete
01:00 - Analysis triggered
11:00 - Fix generated (10 min analysis)
11:30 - REVIEW_*.txt created
[HUMAN REVIEWS - 2-3 minutes]
14:00 - Human approves
14:30 - Approval detected
15:00 - Fix applied
29:00 - Compilation complete (15 min)
29:30 - Commit created
30:00 - PR created
═══════════════════════════════════════
TOTAL: ~30 minutes (27 autonomous + 3 human)
```

### Rejection Path
```
00:00 - Crash occurs
00:30 - Detection complete
01:00 - Analysis triggered
11:00 - Fix generated (10 min analysis)
11:30 - REVIEW_*.txt created
[HUMAN REVIEWS - 2-3 minutes]
14:00 - Human rejects
═══════════════════════════════════════
TOTAL: ~14 minutes (11 autonomous + 3 human)
NO COMPILATION WASTED ✅
NO COMMITS CREATED ✅
NO PRS CREATED ✅
```

---

## ✅ Success Metrics

System working correctly when:
- ✅ Crash detected within 30 seconds
- ✅ Analysis complete within 10 minutes
- ✅ Fix generated with comprehensive details
- ✅ Review request created with all context
- ✅ Human reviews fix in 2-5 minutes
- ✅ Approved fixes deploy automatically within 20 minutes
- ✅ Rejected fixes NEVER get compiled or committed
- ✅ Zero wasted effort on bad fixes

---

**This is the CORRECT workflow where human reviews the FIX, not the PR.**

🤖 Generated with [Claude Code](https://claude.com/claude-code)
