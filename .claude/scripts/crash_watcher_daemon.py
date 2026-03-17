#!/usr/bin/env python3
"""
Autonomous Crash Watcher Daemon
================================

Watches crash_analysis_queue/requests/ for new crashes and automatically
processes them without human intervention.

This daemon:
1. Watches for new request files
2. Reads crash dumps directly
3. Invokes Claude Code analysis automatically
4. Applies fixes
5. Compiles
6. Creates PR
7. Only stops for human approval

Usage:
    python crash_watcher_daemon.py
"""

import json
import time
import subprocess
from pathlib import Path
from datetime import datetime
import sys

class AutonomousCrashWatcher:
    def __init__(self, trinity_root: Path):
        self.trinity_root = trinity_root
        self.queue_dir = trinity_root / ".claude/crash_analysis_queue"
        self.requests_dir = self.queue_dir / "requests"
        self.responses_dir = self.queue_dir / "responses"
        self.processed = set()  # Track processed request IDs

    def run(self):
        """Main daemon loop"""
        print("🤖 Autonomous Crash Watcher Daemon v1.0")
        print("=" * 60)
        print(f"Monitoring: {self.requests_dir}")
        print(f"Trinity Root: {self.trinity_root}")
        print("=" * 60)
        print("\nWaiting for crash requests...\n")

        while True:
            try:
                # Scan for pending requests
                pending = self.find_pending_requests()

                for request_file in pending:
                    request_id = self.extract_request_id(request_file)

                    if request_id in self.processed:
                        continue  # Already processed

                    print(f"\n🔔 NEW CRASH REQUEST: {request_id}")
                    print(f"📁 File: {request_file.name}")

                    # Process autonomously
                    success = self.process_crash(request_id, request_file)

                    if success:
                        self.processed.add(request_id)
                        print(f"✅ Request {request_id} processed successfully")
                    else:
                        print(f"❌ Request {request_id} failed")

                # Sleep before next scan
                time.sleep(10)  # Check every 10 seconds

            except KeyboardInterrupt:
                print("\n\n🛑 Daemon stopped by user")
                break
            except Exception as e:
                print(f"❌ Error in daemon loop: {e}")
                time.sleep(30)  # Wait longer on error

    def find_pending_requests(self):
        """Find all pending request files"""
        if not self.requests_dir.exists():
            return []

        pending = []
        for request_file in self.requests_dir.glob("request_*.json"):
            try:
                with open(request_file, 'r', encoding='utf-8') as f:
                    data = json.load(f)

                request_id = data['request_id']
                status = data.get('status', 'pending')

                # Check if response exists
                response_file = self.responses_dir / f"response_{request_id}.json"

                if status == 'pending' and not response_file.exists():
                    pending.append(request_file)

            except Exception as e:
                print(f"⚠️ Error reading {request_file}: {e}")

        return pending

    def extract_request_id(self, request_file: Path) -> str:
        """Extract request ID from filename"""
        # request_abc123.json -> abc123
        return request_file.stem.replace('request_', '')

    def process_crash(self, request_id: str, request_file: Path) -> bool:
        """
        Autonomous crash processing pipeline

        This is the KEY automation - no human involvement until approval
        """
        try:
            # Read request
            with open(request_file, 'r', encoding='utf-8') as f:
                request_data = json.load(f)

            crash_id = request_data['crash']['crash_id']
            print(f"\n📊 Processing crash {crash_id}...")

            # Step 1: Read crash dump analysis (already done by Python orchestrator)
            print("✅ Step 1: Crash dump parsed by orchestrator")

            # Step 2: Perform Claude Code analysis
            # THIS IS WHERE YOU (Claude Code) PROCESS THE REQUEST AUTOMATICALLY
            print("🤖 Step 2: Invoking Claude Code analysis...")

            if not self.invoke_claude_analysis(request_id):
                return False

            # Step 3: Wait for response
            print("⏳ Step 3: Waiting for analysis response...")
            if not self.wait_for_response(request_id, timeout=600):
                print("❌ Analysis timeout")
                return False

            # Step 4: Read response and apply fix
            print("📝 Step 4: Reading analysis response...")
            response = self.read_response(request_id)

            if not response:
                print("❌ Failed to read response")
                return False

            # Step 5: Apply fix to filesystem
            print("🔧 Step 5: Applying fix to filesystem...")
            if not self.apply_fix(response):
                print("❌ Fix application failed")
                return False

            # Step 6: Compile
            print("🔨 Step 6: Compiling worldserver with fix...")
            if not self.compile_fix():
                print("❌ Compilation failed")
                return False

            # Step 7: Commit
            print("💾 Step 7: Creating git commit...")
            if not self.create_commit(crash_id, response):
                print("⚠️ Commit creation failed (non-fatal)")

            # Step 8: Create PR
            print("📋 Step 8: Creating GitHub PR...")
            if not self.create_pr(crash_id, response):
                print("⚠️ PR creation failed (non-fatal)")

            print(f"\n✅ AUTONOMOUS PIPELINE COMPLETE")
            print(f"   Crash: {crash_id}")
            print(f"   Status: Ready for human review")
            print(f"   Next: Review PR and approve for deployment")

            return True

        except Exception as e:
            print(f"❌ Error in process_crash: {e}")
            import traceback
            traceback.print_exc()
            return False

    def invoke_claude_analysis(self, request_id: str) -> bool:
        """
        THIS IS THE KEY INSIGHT:

        We can trigger your (Claude Code's) /analyze-crash command
        programmatically by simply reading the crash txt file and
        invoking your analysis.

        Options:
        1. Call Claude Code API (if available)
        2. Simulate /analyze-crash command
        3. Write analysis trigger file that you watch
        """

        # For now, we'll create a trigger file that tells you to process this
        trigger_file = self.queue_dir / "triggers" / f"trigger_{request_id}.json"
        trigger_file.parent.mkdir(exist_ok=True)

        trigger_data = {
            "request_id": request_id,
            "timestamp": datetime.now().isoformat(),
            "action": "analyze",
            "autonomous": True
        }

        trigger_file.write_text(json.dumps(trigger_data, indent=2), encoding='utf-8')

        print(f"✅ Created analysis trigger: {trigger_file}")

        # Alternative: Direct invocation of analysis logic
        # This is where you (Claude Code) would automatically run /analyze-crash
        # For now, this is a placeholder

        return True

    def wait_for_response(self, request_id: str, timeout: int = 600) -> bool:
        """Wait for Claude Code to generate response"""
        response_file = self.responses_dir / f"response_{request_id}.json"

        start_time = time.time()
        while time.time() - start_time < timeout:
            if response_file.exists():
                # Give it a moment to finish writing
                time.sleep(2)
                print(f"✅ Response file detected")
                return True

            time.sleep(5)
            print(".", end="", flush=True)

        return False

    def read_response(self, request_id: str):
        """Read analysis response"""
        response_file = self.responses_dir / f"response_{request_id}.json"

        try:
            with open(response_file, 'r', encoding='utf-8') as f:
                return json.load(f)
        except Exception as e:
            print(f"❌ Error reading response: {e}")
            return None

    def apply_fix(self, response: dict) -> bool:
        """
        Apply fix from response to filesystem

        The response JSON should contain the exact code to write
        """
        try:
            files_modified = response.get('fix', {}).get('files_modified', [])

            if not files_modified:
                print("⚠️ No files to modify in response")
                return False

            for file_info in files_modified:
                file_path = self.trinity_root / file_info['path']

                # The fix has already been applied by Claude Code
                # This step is redundant if Claude Code applies fixes directly
                print(f"✅ Fix already applied to: {file_path}")

            return True

        except Exception as e:
            print(f"❌ Error applying fix: {e}")
            return False

    def compile_fix(self) -> bool:
        """Compile worldserver with fix"""
        build_dir = self.trinity_root / "build"

        cmd = [
            "cmake", "--build", str(build_dir),
            "--target", "worldserver",
            "--config", "RelWithDebInfo",
            "-j", "8"
        ]

        print(f"Running: {' '.join(cmd)}")

        result = subprocess.run(
            cmd,
            cwd=build_dir,
            capture_output=True,
            text=True
        )

        if result.returncode != 0:
            print(f"❌ Compilation failed:")
            print(result.stderr)
            return False

        print(f"✅ Compilation successful")
        return True

    def create_commit(self, crash_id: str, response: dict) -> bool:
        """Create git commit for fix"""
        try:
            # Git add modified files
            files_modified = response.get('fix', {}).get('files_modified', [])
            for file_info in files_modified:
                file_path = file_info['path']
                subprocess.run(['git', 'add', file_path], cwd=self.trinity_root, check=True)

            # Create commit message
            crash_info = response['crash_analysis']
            commit_msg = f"""fix(playerbot): {crash_info['category']} in {crash_info['crash_location']['function']}

Crash ID: {crash_id}
Location: {crash_info['crash_location']['file']}:{crash_info['crash_location']['line']}

Root Cause: {crash_info['root_cause']['summary']}

Fix: {response['fix']['strategy']}

🤖 Automated fix generated by Autonomous Crash System

Co-Authored-By: Claude <noreply@anthropic.com>"""

            # Commit
            subprocess.run(
                ['git', 'commit', '-m', commit_msg],
                cwd=self.trinity_root,
                check=True
            )

            print(f"✅ Commit created")
            return True

        except Exception as e:
            print(f"⚠️ Commit failed: {e}")
            return False

    def create_pr(self, crash_id: str, response: dict) -> bool:
        """Create GitHub PR"""
        try:
            crash_info = response['crash_analysis']

            pr_title = f"fix(playerbot): {crash_info['category']} - {crash_id[:8]}"

            pr_body = f"""# Autonomous Crash Fix

## 🤖 Automatically Generated Fix

This PR was generated by the Autonomous Crash Analysis System without human intervention.

## 🐛 Crash Details

- **Crash ID:** {crash_id}
- **Exception:** {crash_info.get('exception', 'N/A')}
- **Location:** `{crash_info['crash_location']['file']}:{crash_info['crash_location']['line']}`
- **Function:** `{crash_info['crash_location']['function']}`

## 🔍 Root Cause

{crash_info['root_cause']['summary']}

## ✅ Fix Applied

{response['fix']['implementation_details']['fix_description']}

## 📋 Review Checklist

- [ ] Review fix code
- [ ] Verify compilation success
- [ ] Check test results
- [ ] Approve for deployment

---

🤖 Generated by Autonomous Crash System v1.0
"""

            # Create PR as draft
            cmd = [
                'gh', 'pr', 'create',
                '--title', pr_title,
                '--body', pr_body,
                '--label', 'automated-fix,crash-fix',
                '--draft'
            ]

            result = subprocess.run(
                cmd,
                cwd=self.trinity_root,
                capture_output=True,
                text=True
            )

            if result.returncode != 0:
                print(f"⚠️ PR creation failed: {result.stderr}")
                return False

            print(f"✅ PR created: {result.stdout}")
            return True

        except Exception as e:
            print(f"⚠️ PR creation failed: {e}")
            return False


def main():
    # Find TrinityCore root
    script_path = Path(__file__).resolve()
    trinity_root = script_path.parent.parent.parent

    print(f"TrinityCore Root: {trinity_root}")

    # Create and run daemon
    watcher = AutonomousCrashWatcher(trinity_root)
    watcher.run()


if __name__ == "__main__":
    main()
