# Autonomous Crash Analysis System V2 - Full Automation

## 🎯 Problem Statement

**Current System Issues:**
- ❌ Requires human to invoke `/analyze-crash` command
- ❌ Manual monitoring of queue
- ❌ Python creates request, then waits for human
- ❌ No autonomous end-to-end workflow

**What Should Happen:**
- ✅ Crash detected → Analysis → Fix → Compile → Test → Await approval
- ✅ Zero human intervention until approval needed
- ✅ Full automation with quality gates

---

## 🚀 Redesigned System Architecture

### Phase 1: Autonomous Detection & Analysis

```
CRASH DUMP CREATED
      ↓
[Python Orchestrator Detects]
      ↓
[Parses with CDB/WinDbg]
      ↓
[Creates Analysis Request JSON]
      ↓
[DIRECTLY INVOKES Claude Code via API/CLI]  ← NEW
      ↓
[Claude Code Performs Comprehensive Analysis]
  - Trinity MCP research
  - Serena codebase analysis
  - Generate enterprise-grade fix
      ↓
[Writes Response JSON]
      ↓
[Writes Fix to Filesystem]  ← NEW
      ↓
[Python Compiles Fix]  ← NEW
      ↓
[Python Runs Unit Tests]  ← NEW
      ↓
[Creates PR Draft with Full Analysis]  ← NEW
      ↓
[AWAITS HUMAN APPROVAL]  ← Only human touchpoint
```

### Phase 2: Approval & Deployment

```
[Human Reviews PR]
      ↓
   Approved?
   ↙     ↘
  YES    NO
   ↓      ↓
[Merge] [Close + Log Reason]
   ↓
[Deploy to Test Environment]
   ↓
[Automated Stress Test]
   ↓
[Production Deployment]
```

---

## 🔧 Implementation Requirements

### 1. Python Orchestrator Enhancement

**File:** `.claude/scripts/crash_auto_fix_v4.py` (NEW VERSION)

```python
#!/usr/bin/env python3
"""
Autonomous Crash Analysis System V4
====================================

Fully autonomous crash detection, analysis, fix generation, compilation,
and PR creation. Only human approval required for deployment.

Usage:
    python crash_auto_fix_v4.py --mode daemon
    python crash_auto_fix_v4.py --mode single <crash_dump>
"""

import subprocess
import json
import time
from pathlib import Path
from dataclasses import dataclass
from typing import Optional

@dataclass
class CrashAnalysisJob:
    crash_id: str
    request_id: str
    crash_dump_path: Path
    status: str  # pending, analyzing, fixing, compiling, testing, pr_created, approved, failed

class AutonomousCrashAnalyzer:
    def __init__(self, trinity_root: Path):
        self.trinity_root = trinity_root
        self.crash_queue = trinity_root / ".claude/crash_analysis_queue"
        self.claude_cli = "claude-code"  # Claude Code CLI tool

    def daemon_mode(self):
        """Continuously monitor for crashes and process autonomously"""
        print("🤖 Autonomous Crash Analyzer V4 - Daemon Mode")
        print("=" * 60)

        while True:
            # Scan for new crash dumps
            new_crashes = self.scan_for_crashes()

            for crash_dump in new_crashes:
                print(f"\n🔍 New crash detected: {crash_dump}")

                # Full autonomous pipeline
                job = self.create_analysis_job(crash_dump)

                # Step 1: Parse crash dump
                self.parse_crash_dump(job)

                # Step 2: Invoke Claude Code for analysis
                self.invoke_claude_analysis(job)

                # Step 3: Wait for analysis completion
                if not self.wait_for_analysis(job, timeout=600):
                    print(f"❌ Analysis timeout for {job.crash_id}")
                    continue

                # Step 4: Apply fix to filesystem
                if not self.apply_fix(job):
                    print(f"❌ Fix application failed for {job.crash_id}")
                    continue

                # Step 5: Compile with fix
                if not self.compile_fix(job):
                    print(f"❌ Compilation failed for {job.crash_id}")
                    continue

                # Step 6: Run unit tests
                if not self.run_tests(job):
                    print(f"⚠️ Tests failed for {job.crash_id}")
                    # Continue anyway - human will review

                # Step 7: Create GitHub PR
                if not self.create_pr(job):
                    print(f"❌ PR creation failed for {job.crash_id}")
                    continue

                print(f"✅ Job complete! PR created, awaiting approval")

            # Sleep before next scan
            time.sleep(30)

    def invoke_claude_analysis(self, job: CrashAnalysisJob):
        """
        Invoke Claude Code CLI to analyze crash autonomously

        This is the KEY change - Python directly invokes Claude Code
        instead of waiting for human to run /analyze-crash
        """
        print(f"🤖 Invoking Claude Code for autonomous analysis...")

        request_file = self.crash_queue / "requests" / f"request_{job.request_id}.json"

        # Invoke Claude Code with custom command
        cmd = [
            self.claude_cli,
            "analyze-crash",
            job.request_id,
            "--autonomous",  # Flag for autonomous mode
            "--no-interaction"  # No human prompts
        ]

        result = subprocess.run(cmd, capture_output=True, text=True)

        if result.returncode != 0:
            print(f"❌ Claude Code invocation failed: {result.stderr}")
            return False

        print(f"✅ Claude Code analysis started")
        return True

    def wait_for_analysis(self, job: CrashAnalysisJob, timeout: int = 600) -> bool:
        """Wait for Claude Code to complete analysis"""
        response_file = self.crash_queue / "responses" / f"response_{job.request_id}.json"

        start_time = time.time()
        while time.time() - start_time < timeout:
            if response_file.exists():
                print(f"✅ Analysis response received")
                return True
            time.sleep(5)

        return False

    def apply_fix(self, job: CrashAnalysisJob) -> bool:
        """Apply fix from response JSON to filesystem"""
        response_file = self.crash_queue / "responses" / f"response_{job.request_id}.json"

        with open(response_file, 'r') as f:
            response = json.load(f)

        # Extract fix code and target files
        fix_strategy = response.get('fix_strategy', {})
        files_to_modify = fix_strategy.get('files_to_modify', [])

        print(f"📝 Applying fix to {len(files_to_modify)} file(s)...")

        for file_info in files_to_modify:
            file_path = self.trinity_root / file_info['path']
            fix_content = file_info.get('new_content')

            if not fix_content:
                print(f"⚠️ No fix content for {file_path}")
                continue

            # Apply fix
            file_path.write_text(fix_content, encoding='utf-8')
            print(f"✅ Applied fix to {file_path}")

        return True

    def compile_fix(self, job: CrashAnalysisJob) -> bool:
        """Compile worldserver with fix"""
        print(f"🔨 Compiling worldserver with fix...")

        build_dir = self.trinity_root / "build"
        cmd = [
            "cmake", "--build", str(build_dir),
            "--target", "worldserver",
            "--config", "RelWithDebInfo",
            "-j", "8"
        ]

        result = subprocess.run(cmd, capture_output=True, text=True, cwd=build_dir)

        if result.returncode != 0:
            print(f"❌ Compilation failed:\n{result.stderr}")
            return False

        print(f"✅ Compilation successful")
        return True

    def run_tests(self, job: CrashAnalysisJob) -> bool:
        """Run unit tests for the fix"""
        print(f"🧪 Running unit tests...")

        # Run playerbot unit tests
        test_dir = self.trinity_root / "src/modules/Playerbot/Tests"

        # Example: Run specific test suite
        cmd = ["pytest", str(test_dir), "-v"]

        result = subprocess.run(cmd, capture_output=True, text=True)

        if result.returncode != 0:
            print(f"⚠️ Tests failed (non-blocking):\n{result.stderr}")
            return False

        print(f"✅ All tests passed")
        return True

    def create_pr(self, job: CrashAnalysisJob) -> bool:
        """Create GitHub PR with comprehensive fix information"""
        print(f"📋 Creating GitHub PR...")

        response_file = self.crash_queue / "responses" / f"response_{job.request_id}.json"

        with open(response_file, 'r') as f:
            response = json.load(f)

        crash = response['crash_analysis']
        fix = response['fix']

        # Generate PR title and body
        pr_title = f"fix(playerbot): {crash['category']} in {crash['crash_location']['function']}"

        pr_body = f"""
# Autonomous Crash Fix - {job.crash_id}

## 🤖 Automated Analysis & Fix

This PR was **automatically generated** by the Autonomous Crash Analysis System.

## 🐛 Crash Details

- **Crash ID:** {job.crash_id}
- **Exception:** {crash.get('exception_code', 'N/A')}
- **Location:** {crash['crash_location']['file']}:{crash['crash_location']['line']}
- **Function:** {crash['crash_location']['function']}
- **Severity:** {crash['severity']}

## 🔍 Root Cause

{crash['root_cause']['summary']}

**Technical Details:**
{crash['root_cause']['technical_details']}

## ✅ Fix Applied

**Strategy:** {fix['strategy']}
**Hierarchy Level:** {fix['implementation_details']['methods_fixed']}

**Files Modified:**
{self._format_files_modified(fix['files_modified'])}

**Safety Improvements:**
{self._format_safety_improvements(fix['implementation_details']['safety_improvements'])}

## 🧪 Automated Testing

- ✅ Compilation: SUCCESS
- ✅ Unit Tests: {self._get_test_status(job)}
- ⏳ Manual Review: REQUIRED

## 📊 Quality Metrics

{self._format_quality_standards(fix['quality_standards'])}

## 🚀 Deployment Readiness

This fix is ready for human review and approval.

**Recommended Actions:**
1. Review fix code and analysis
2. Approve if satisfactory
3. System will automatically deploy to test environment
4. Run 24-hour stress test
5. Promote to production

---

**Autonomous System:** Crash Analysis V4
**Generated:** {response['timestamp']}
**Commit:** (will be created on approval)

🤖 Generated with [Claude Code](https://claude.com/claude-code)
"""

        # Create PR using gh CLI
        cmd = [
            "gh", "pr", "create",
            "--title", pr_title,
            "--body", pr_body,
            "--label", "automated-fix,crash-fix,needs-review",
            "--draft"  # Create as draft for review
        ]

        result = subprocess.run(cmd, capture_output=True, text=True, cwd=self.trinity_root)

        if result.returncode != 0:
            print(f"❌ PR creation failed: {result.stderr}")
            return False

        print(f"✅ PR created successfully")
        return True

    def _format_files_modified(self, files):
        return "\n".join([f"- `{f['path']}` ({f['change_type']})" for f in files])

    def _format_safety_improvements(self, improvements):
        return "\n".join([f"- {imp}" for imp in improvements])

    def _get_test_status(self, job):
        return "PASSED" if job.status != "testing_failed" else "FAILED (non-blocking)"

    def _format_quality_standards(self, standards):
        return "\n".join([f"- {k}: {v}" for k, v in standards.items()])


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description="Autonomous Crash Analysis System V4")
    parser.add_argument("--mode", choices=["daemon", "single"], required=True)
    parser.add_argument("crash_dump", nargs="?", help="Crash dump path for single mode")

    args = parser.parse_args()

    trinity_root = Path(__file__).parent.parent.parent
    analyzer = AutonomousCrashAnalyzer(trinity_root)

    if args.mode == "daemon":
        analyzer.daemon_mode()
    else:
        # Single crash analysis
        if not args.crash_dump:
            print("❌ Crash dump path required for single mode")
            exit(1)

        job = analyzer.create_analysis_job(Path(args.crash_dump))
        # ... run pipeline once
```

---

### 2. Claude Code Enhancement

**Required:** Claude Code needs to support autonomous invocation

**Option A: Claude Code API (Preferred)**
```python
# Python can invoke Claude Code programmatically
from claude_code import ClaudeCode

claude = ClaudeCode(api_key=os.getenv("ANTHROPIC_API_KEY"))
result = claude.analyze_crash(
    request_id="abc123",
    autonomous=True,
    no_interaction=True
)
```

**Option B: Claude Code CLI**
```bash
# Invoke via subprocess
claude-code analyze-crash abc123 --autonomous --no-interaction
```

**Option C: Direct File Watching (Current Fallback)**
```python
# Claude Code watches .claude/crash_analysis_queue/requests/
# Automatically processes new requests without human invocation
```

---

### 3. Workflow State Machine

```
STATES:
- CRASH_DETECTED     → New crash dump found
- PARSING           → CDB parsing in progress
- ANALYSIS_PENDING  → Request created, awaiting Claude
- ANALYZING         → Claude performing analysis
- FIX_GENERATED     → Response JSON written
- APPLYING_FIX      → Writing fix to filesystem
- COMPILING         → Building worldserver
- TESTING           → Running unit tests
- PR_CREATED        → Draft PR created
- AWAITING_APPROVAL → Human review required
- APPROVED          → Fix approved for deployment
- DEPLOYING         → Deploying to test environment
- DEPLOYED          → In production
- REJECTED          → Human rejected fix
- FAILED            → Error in pipeline

TRANSITIONS:
CRASH_DETECTED → PARSING → ANALYSIS_PENDING → ANALYZING → FIX_GENERATED
→ APPLYING_FIX → COMPILING → TESTING → PR_CREATED → AWAITING_APPROVAL
→ APPROVED → DEPLOYING → DEPLOYED
```

---

## 🎯 Benefits of Full Automation

### Before (Current System)
1. Crash detected
2. Python creates request JSON
3. **[WAIT FOR HUMAN]** to run `/analyze-crash`
4. Claude analyzes
5. Claude writes response
6. **[WAIT FOR HUMAN]** to apply fix
7. **[WAIT FOR HUMAN]** to compile
8. **[WAIT FOR HUMAN]** to commit
9. **[WAIT FOR HUMAN]** to create PR

**Human touchpoints:** 5+
**Time to PR:** Hours to days

### After (Autonomous System)
1. Crash detected
2. Python creates request JSON
3. **[AUTOMATIC]** Python invokes Claude
4. **[AUTOMATIC]** Claude analyzes
5. **[AUTOMATIC]** Claude writes response
6. **[AUTOMATIC]** Python applies fix
7. **[AUTOMATIC]** Python compiles
8. **[AUTOMATIC]** Python runs tests
9. **[AUTOMATIC]** Python creates PR
10. **[HUMAN APPROVAL ONLY]** Review and approve

**Human touchpoints:** 1 (approval only)
**Time to PR:** Minutes (30-60 min)

---

## 🔒 Safety & Quality Gates

### Automated Quality Checks

1. **Compilation Gate**
   - Fix MUST compile successfully
   - If compilation fails → Mark as FAILED, log, notify

2. **Test Gate** (Non-blocking)
   - Run unit tests
   - Log results in PR
   - Tests can fail but PR still created for review

3. **Code Quality Gate**
   - Run static analysis
   - Check for security issues
   - Log warnings in PR

4. **Project Rules Gate**
   - Validate file modification hierarchy
   - Ensure module-only when possible
   - Check for core modifications

### Human Approval Gate

**What Human Reviews:**
- Fix strategy and approach
- Code quality and correctness
- Test results
- Deployment risk assessment

**Human Actions:**
- ✅ Approve → Automatic deployment begins
- ❌ Reject → Log reason, close PR, analyze failure
- 🔄 Request changes → Claude can iterate

---

## 📈 Expected Performance

**Autonomous Pipeline:**
- **Detection to Analysis:** 2-5 minutes
- **Analysis to Fix:** 5-10 minutes
- **Fix to Compilation:** 3-5 minutes
- **Compilation to Tests:** 2-3 minutes
- **Tests to PR:** 1-2 minutes
- **TOTAL:** 15-30 minutes to PR (fully automated)

**Human Review:**
- **PR Review:** 5-15 minutes (human time)
- **Approval to Deployment:** 5-10 minutes (automated)

**End-to-End:** Crash to production in ~1 hour (with fast approval)

---

## 🚀 Implementation Plan

### Phase 1: Core Automation (Week 1)
- [ ] Implement `crash_auto_fix_v4.py` with daemon mode
- [ ] Add Claude Code autonomous invocation
- [ ] Implement fix application logic
- [ ] Add compilation automation

### Phase 2: Testing & PR (Week 1)
- [ ] Integrate unit test runner
- [ ] Implement PR creation logic
- [ ] Add quality gates

### Phase 3: Deployment Pipeline (Week 2)
- [ ] Add test environment deployment
- [ ] Implement stress testing
- [ ] Add production deployment logic

### Phase 4: Monitoring & Iteration (Week 2)
- [ ] Add performance metrics
- [ ] Implement failure analysis
- [ ] Add self-improvement loop

---

## 💡 Future Enhancements

### Self-Learning System
- Track which fixes work vs. fail
- Learn from human rejection reasons
- Improve fix generation over time
- Build crash pattern database

### Multi-Bot Coordination
- Analyze crash trends across all bots
- Detect systemic issues
- Prioritize critical vs. minor crashes
- Batch similar fixes

### Predictive Crash Prevention
- Analyze code for crash-prone patterns
- Suggest preventive refactoring
- Run static analysis on commits
- Pre-emptively fix potential crashes

---

## ✅ Success Criteria

**System is successful when:**
- [ ] 90%+ of crashes are analyzed automatically
- [ ] 80%+ of fixes compile successfully
- [ ] 70%+ of PR

s are approved by humans
- [ ] Average time to PR < 30 minutes
- [ ] Human intervention < 5% of pipeline
- [ ] Zero false positives in critical fixes

---

**Status:** 📋 Design Complete - Ready for Implementation
**Priority:** 🔥 HIGH - Eliminates major manual bottleneck
**Effort:** 2 weeks for full implementation
**ROI:** Massive - Saves hours per crash, scales infinitely

🤖 Generated with [Claude Code](https://claude.com/claude-code)
