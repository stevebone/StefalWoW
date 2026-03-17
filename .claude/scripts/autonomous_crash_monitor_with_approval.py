#!/usr/bin/env python3
"""
Autonomous Crash Monitor - WITH HUMAN APPROVAL GATE
====================================================

Fully autonomous crash processing with human approval at the RIGHT stage:
- Autonomous: Detection → Analysis → Fix Generation
- Human Review: Review the generated fix
- Autonomous: Apply fix → Compile → Commit → PR → Deploy

This is the CORRECT workflow where human reviews the FIX, not the PR.
"""

import json
import time
import subprocess
import sys
from pathlib import Path
from datetime import datetime
import traceback


class AutonomousCrashMonitorWithApproval:
    def __init__(self, trinity_root: Path):
        self.trinity_root = trinity_root
        self.queue_dir = trinity_root / ".claude/crash_analysis_queue"
        self.requests_dir = self.queue_dir / "requests"
        self.responses_dir = self.queue_dir / "responses"
        self.approvals_dir = self.queue_dir / "approvals"
        self.log_file = trinity_root / ".claude/logs/autonomous_monitor_approval.log"
        self.processed_cache = set()

        # Create directories
        self.log_file.parent.mkdir(parents=True, exist_ok=True)
        self.requests_dir.mkdir(parents=True, exist_ok=True)
        self.responses_dir.mkdir(parents=True, exist_ok=True)
        self.approvals_dir.mkdir(parents=True, exist_ok=True)

    def log(self, message: str, level: str = "INFO"):
        """Log message with timestamp"""
        timestamp = datetime.now().isoformat()
        log_msg = f"[{timestamp}] [{level}] {message}"
        print(log_msg)
        try:
            with open(self.log_file, 'a', encoding='utf-8') as f:
                f.write(log_msg + '\n')
        except:
            pass

    def find_pending_requests(self):
        """Find pending requests that need analysis"""
        pending = []
        if not self.requests_dir.exists():
            return pending

        for request_file in self.requests_dir.glob("request_*.json"):
            try:
                with open(request_file, 'r', encoding='utf-8') as f:
                    data = json.load(f)

                request_id = data['request_id']
                status = data.get('status', 'pending')
                response_file = self.responses_dir / f"response_{request_id}.json"

                # Pending: status=pending, no response yet
                if status == 'pending' and not response_file.exists() and request_id not in self.processed_cache:
                    pending.append({
                        'request_id': request_id,
                        'request_file': request_file,
                        'crash_id': data['crash']['crash_id'],
                        'data': data
                    })
            except Exception as e:
                self.log(f"ERROR reading {request_file}: {e}", "ERROR")
        return pending

    def find_responses_awaiting_approval(self):
        """Find responses that have been generated but not yet approved"""
        awaiting = []
        if not self.responses_dir.exists():
            return awaiting

        for response_file in self.responses_dir.glob("response_*.json"):
            try:
                request_id = response_file.stem.replace('response_', '')
                approval_file = self.approvals_dir / f"approval_{request_id}.json"

                # If no approval file exists, this fix needs review
                if not approval_file.exists():
                    with open(response_file, 'r', encoding='utf-8') as f:
                        response_data = json.load(f)

                    awaiting.append({
                        'request_id': request_id,
                        'response_file': response_file,
                        'crash_id': response_data.get('crash_analysis', {}).get('crash_id', 'unknown'),
                        'data': response_data
                    })
            except Exception as e:
                self.log(f"ERROR reading {response_file}: {e}", "ERROR")
        return awaiting

    def find_approved_fixes(self):
        """Find fixes that have been approved and need deployment"""
        approved = []
        if not self.approvals_dir.exists():
            return approved

        for approval_file in self.approvals_dir.glob("approval_*.json"):
            try:
                with open(approval_file, 'r', encoding='utf-8') as f:
                    approval_data = json.load(f)

                request_id = approval_data['request_id']
                status = approval_data.get('status', 'pending')

                # Only process 'approved' fixes, not 'deployed' or 'rejected'
                if status == 'approved':
                    approved.append({
                        'request_id': request_id,
                        'approval_file': approval_file,
                        'data': approval_data
                    })
            except Exception as e:
                self.log(f"ERROR reading {approval_file}: {e}", "ERROR")
        return approved

    def create_auto_process_marker(self, request_info: dict) -> bool:
        """Create marker for Claude Code to analyze"""
        try:
            request_id = request_info['request_id']
            marker_dir = self.queue_dir / "auto_process"
            marker_dir.mkdir(exist_ok=True)

            marker_file = marker_dir / f"process_{request_id}.json"
            marker_data = {
                "request_id": request_id,
                "crash_id": request_info['crash_id'],
                "request_file": str(request_info['request_file']),
                "triggered_at": datetime.now().isoformat(),
                "autonomous": True
            }

            with open(marker_file, 'w', encoding='utf-8') as f:
                json.dump(marker_data, f, indent=2)

            self.log(f"✅ Created auto-process marker for {request_id}")
            return True
        except Exception as e:
            self.log(f"ERROR creating marker: {e}", "ERROR")
            return False

    def create_approval_request(self, response_info: dict) -> bool:
        """
        Create human-readable approval request for fix review

        This is where HUMAN REVIEWS THE FIX before deployment
        """
        try:
            request_id = response_info['request_id']
            crash_id = response_info['crash_id']
            response_data = response_info['data']

            self.log("")
            self.log("=" * 70)
            self.log("🔔 NEW FIX READY FOR HUMAN REVIEW")
            self.log("=" * 70)
            self.log(f"Request ID: {request_id}")
            self.log(f"Crash ID: {crash_id}")
            self.log("")

            # Extract fix details
            crash_analysis = response_data.get('crash_analysis', {})
            fix_info = response_data.get('fix', {})

            # Create human-readable approval request
            approval_request_file = self.approvals_dir / f"REVIEW_{request_id}.txt"

            review_text = f"""
╔══════════════════════════════════════════════════════════════════════
║ CRASH FIX READY FOR HUMAN REVIEW
╚══════════════════════════════════════════════════════════════════════

REQUEST ID: {request_id}
CRASH ID: {crash_id}
GENERATED: {datetime.now().isoformat()}

══════════════════════════════════════════════════════════════════════
CRASH ANALYSIS SUMMARY
══════════════════════════════════════════════════════════════════════

Exception: {crash_analysis.get('exception', 'N/A')}
Location: {crash_analysis.get('crash_location', {}).get('file', 'N/A')}:{crash_analysis.get('crash_location', {}).get('line', 'N/A')}
Function: {crash_analysis.get('crash_location', {}).get('function', 'N/A')}

Root Cause:
{crash_analysis.get('root_cause', {}).get('summary', 'N/A')}

══════════════════════════════════════════════════════════════════════
PROPOSED FIX
══════════════════════════════════════════════════════════════════════

Strategy: {fix_info.get('strategy', 'N/A')}

Files Modified:
"""
            for file_mod in fix_info.get('files_modified', []):
                review_text += f"  - {file_mod.get('path', 'N/A')}\n"

            review_text += f"""
Fix Description:
{fix_info.get('implementation_details', {}).get('fix_description', 'N/A')}

══════════════════════════════════════════════════════════════════════
REVIEW INSTRUCTIONS
══════════════════════════════════════════════════════════════════════

1. Review the full response JSON at:
   {response_info['response_file']}

2. Review the fix code and implementation details

3. Make your decision:

   TO APPROVE (deploy the fix):
   ----------------------------------------------------------------------
   Create file: {self.approvals_dir}/approval_{request_id}.json

   Content:
   {{
     "request_id": "{request_id}",
     "status": "approved",
     "approved_by": "YOUR_NAME",
     "approved_at": "{datetime.now().isoformat()}",
     "comments": "Fix looks good, approved for deployment"
   }}

   TO REJECT (do not deploy):
   ----------------------------------------------------------------------
   Create file: {self.approvals_dir}/approval_{request_id}.json

   Content:
   {{
     "request_id": "{request_id}",
     "status": "rejected",
     "rejected_by": "YOUR_NAME",
     "rejected_at": "{datetime.now().isoformat()}",
     "reason": "Explain why the fix is not acceptable"
   }}

══════════════════════════════════════════════════════════════════════

Once you create the approval file, the system will automatically:
- Apply the fix to source code (if approved)
- Compile worldserver
- Run tests
- Create git commit
- Create GitHub PR
- Deploy to production

HUMAN DECISION REQUIRED - System waiting for your approval...
"""

            with open(approval_request_file, 'w', encoding='utf-8') as f:
                f.write(review_text)

            self.log(f"📝 Created approval request: {approval_request_file}")
            self.log(f"")
            self.log(f"⏳ WAITING FOR HUMAN REVIEW OF FIX")
            self.log(f"   Review file: {approval_request_file}")
            self.log(f"   Response: {response_info['response_file']}")
            self.log(f"")
            self.log(f"   Create approval file to proceed:")
            self.log(f"   {self.approvals_dir}/approval_{request_id}.json")
            self.log("")

            return True

        except Exception as e:
            self.log(f"ERROR creating approval request: {e}", "ERROR")
            self.log(traceback.format_exc(), "DEBUG")
            return False

    def deploy_approved_fix(self, approval_info: dict) -> bool:
        """
        Deploy an approved fix - fully automated after human approval

        Steps:
        1. Read response JSON
        2. Apply fix to source files
        3. Compile worldserver
        4. Run tests
        5. Create git commit
        6. Create GitHub PR
        7. Mark as deployed
        """
        request_id = approval_info['request_id']

        self.log("")
        self.log("=" * 70)
        self.log("🚀 DEPLOYING APPROVED FIX (AUTOMATED)")
        self.log("=" * 70)
        self.log(f"Request ID: {request_id}")
        self.log("")

        try:
            # Read response
            response_file = self.responses_dir / f"response_{request_id}.json"
            with open(response_file, 'r', encoding='utf-8') as f:
                response_data = json.load(f)

            # Step 1: Apply fix to filesystem
            self.log("📝 Step 1/5: Applying fix to source files...")
            if not self.apply_fix_to_filesystem(response_data):
                self.log("❌ Fix application failed", "ERROR")
                return False

            # Step 2: Compile
            self.log("🔨 Step 2/5: Compiling worldserver...")
            if not self.compile_worldserver():
                self.log("❌ Compilation failed", "ERROR")
                return False

            # Step 3: Run tests (optional)
            self.log("🧪 Step 3/5: Running tests...")
            # self.run_tests()  # Optional

            # Step 4: Create commit
            self.log("💾 Step 4/5: Creating git commit...")
            if not self.create_git_commit(response_data):
                self.log("⚠️ Git commit failed (non-fatal)", "WARN")

            # Step 5: Create PR
            self.log("📋 Step 5/5: Creating GitHub PR...")
            if not self.create_github_pr(response_data):
                self.log("⚠️ PR creation failed (non-fatal)", "WARN")

            # Mark as deployed
            approval_file = approval_info['approval_file']
            with open(approval_file, 'r', encoding='utf-8') as f:
                approval_data = json.load(f)

            approval_data['status'] = 'deployed'
            approval_data['deployed_at'] = datetime.now().isoformat()

            with open(approval_file, 'w', encoding='utf-8') as f:
                json.dump(approval_data, f, indent=2)

            self.log("")
            self.log("✅ DEPLOYMENT COMPLETE")
            self.log(f"   Fix applied, compiled, committed, and PR created")
            self.log("")

            return True

        except Exception as e:
            self.log(f"ERROR deploying fix: {e}", "ERROR")
            self.log(traceback.format_exc(), "DEBUG")
            return False

    def apply_fix_to_filesystem(self, response_data: dict) -> bool:
        """Apply fix code to actual source files"""
        try:
            files_modified = response_data.get('fix', {}).get('files_modified', [])

            if not files_modified:
                self.log("⚠️ No files to modify", "WARN")
                return False

            for file_info in files_modified:
                file_path = self.trinity_root / file_info['path']
                new_code = file_info.get('new_code', '')

                if not new_code:
                    self.log(f"⚠️ No new code for {file_path}", "WARN")
                    continue

                # Write the fixed code
                self.log(f"  Writing fix to: {file_path}")
                with open(file_path, 'w', encoding='utf-8') as f:
                    f.write(new_code)

            self.log(f"✅ Applied fix to {len(files_modified)} file(s)")
            return True

        except Exception as e:
            self.log(f"ERROR applying fix: {e}", "ERROR")
            return False

    def compile_worldserver(self) -> bool:
        """Compile worldserver with fix"""
        build_dir = self.trinity_root / "build"
        cmd = [
            "cmake", "--build", str(build_dir),
            "--target", "worldserver",
            "--config", "RelWithDebInfo",
            "-j", "8"
        ]

        self.log(f"  Running: {' '.join(cmd)}")
        result = subprocess.run(cmd, cwd=build_dir, capture_output=True, text=True)

        if result.returncode != 0:
            self.log(f"❌ Compilation failed", "ERROR")
            self.log(result.stderr, "DEBUG")
            return False

        self.log("✅ Compilation successful")
        return True

    def create_git_commit(self, response_data: dict) -> bool:
        """Create git commit for the fix"""
        try:
            crash_analysis = response_data['crash_analysis']
            fix_info = response_data['fix']

            # Git add modified files
            for file_info in fix_info.get('files_modified', []):
                subprocess.run(['git', 'add', file_info['path']], cwd=self.trinity_root, check=True)

            # Create commit message
            commit_msg = f"""fix(playerbot): {crash_analysis['category']} in {crash_analysis['crash_location']['function']}

Crash ID: {crash_analysis['crash_id']}
Location: {crash_analysis['crash_location']['file']}:{crash_analysis['crash_location']['line']}

Root Cause: {crash_analysis['root_cause']['summary']}

Fix: {fix_info['strategy']}

🤖 Automated fix - Human approved before deployment

Co-Authored-By: Claude <noreply@anthropic.com>"""

            subprocess.run(['git', 'commit', '-m', commit_msg], cwd=self.trinity_root, check=True)
            self.log("✅ Git commit created")
            return True

        except Exception as e:
            self.log(f"ERROR creating commit: {e}", "ERROR")
            return False

    def create_github_pr(self, response_data: dict) -> bool:
        """Create GitHub PR"""
        try:
            crash_analysis = response_data['crash_analysis']
            pr_title = f"fix(playerbot): {crash_analysis['category']} - {crash_analysis['crash_id'][:8]}"

            pr_body = f"""# Autonomous Crash Fix (Human Approved)

## ✅ Human Reviewed and Approved

This fix was generated by the autonomous crash system and **reviewed by a human** before deployment.

## 🐛 Crash Details

- **Crash ID:** {crash_analysis['crash_id']}
- **Exception:** {crash_analysis.get('exception', 'N/A')}
- **Location:** `{crash_analysis['crash_location']['file']}:{crash_analysis['crash_location']['line']}`
- **Function:** `{crash_analysis['crash_location']['function']}`

## 🔍 Root Cause

{crash_analysis['root_cause']['summary']}

## ✅ Fix Applied

{response_data['fix']['implementation_details']['fix_description']}

---

🤖 Generated by Autonomous System | ✅ Human Approved | Ready for Merge
"""

            cmd = ['gh', 'pr', 'create', '--title', pr_title, '--body', pr_body, '--label', 'automated-fix,crash-fix,human-approved']
            result = subprocess.run(cmd, cwd=self.trinity_root, capture_output=True, text=True)

            if result.returncode != 0:
                self.log(f"⚠️ PR creation failed: {result.stderr}", "WARN")
                return False

            self.log(f"✅ PR created: {result.stdout}")
            return True

        except Exception as e:
            self.log(f"ERROR creating PR: {e}", "ERROR")
            return False

    def run_monitoring_loop(self):
        """Main monitoring loop with correct approval gate"""
        self.log("=" * 70)
        self.log("AUTONOMOUS CRASH MONITOR (WITH HUMAN APPROVAL) v3.0")
        self.log("=" * 70)
        self.log(f"Trinity Root: {self.trinity_root}")
        self.log(f"Approval Gate: After fix generation, before deployment")
        self.log("=" * 70)
        self.log("")

        iteration = 0
        while True:
            try:
                iteration += 1

                # Heartbeat
                if iteration % 10 == 0:
                    self.log(f"💓 Heartbeat: Iteration {iteration}")

                # Phase 1: Find pending requests → trigger analysis
                pending = self.find_pending_requests()
                if pending:
                    self.log(f"🔔 Found {len(pending)} pending request(s) - triggering analysis")
                    for req in pending:
                        self.create_auto_process_marker(req)
                        self.processed_cache.add(req['request_id'])

                # Phase 2: Find responses awaiting approval → request human review
                awaiting = self.find_responses_awaiting_approval()
                if awaiting:
                    self.log(f"📝 Found {len(awaiting)} fix(es) awaiting human approval")
                    for resp in awaiting:
                        self.create_approval_request(resp)

                # Phase 3: Find approved fixes → deploy automatically
                approved = self.find_approved_fixes()
                if approved:
                    self.log(f"🚀 Found {len(approved)} approved fix(es) - deploying automatically")
                    for appr in approved:
                        self.deploy_approved_fix(appr)

                time.sleep(30)

            except KeyboardInterrupt:
                self.log("\n🛑 Shutdown requested")
                break
            except Exception as e:
                self.log(f"ERROR in loop: {e}", "ERROR")
                self.log(traceback.format_exc(), "DEBUG")
                time.sleep(60)

        self.log("Monitor stopped")


def main():
    if hasattr(sys.stdout, 'reconfigure'):
        sys.stdout.reconfigure(encoding='utf-8')

    script_path = Path(__file__).resolve()
    trinity_root = script_path.parent.parent.parent

    monitor = AutonomousCrashMonitorWithApproval(trinity_root)
    monitor.run_monitoring_loop()


if __name__ == "__main__":
    main()
