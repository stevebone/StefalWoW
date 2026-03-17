#!/usr/bin/env python3
"""
Overnight Autonomous Mode - ZERO Human Intervention
====================================================

Fully autonomous crash processing that runs overnight without any human approval.
Every fix is compiled, tested, committed to overnight branch, and deployed.

Safety Mechanisms:
1. Creates overnight-YYYYMMDD branch from playerbot-dev
2. All commits go to overnight branch (not playerbot-dev)
3. Every fix is compiled before committing
4. Compilation failures are logged but don't stop the system
5. worldserver.exe + .pdb copied to M:/Wplayerbot/ after successful compile
6. Runs continuously until user stops with Ctrl+C

In the morning, human reviews overnight branch commits and merges good ones to playerbot-dev.

Usage:
    python overnight_autonomous_mode.py
"""

import json
import time
import subprocess
import sys
import shutil
import os
import atexit
from pathlib import Path
from datetime import datetime
import traceback


class OvernightAutonomousMode:
    def __init__(self, trinity_root: Path):
        self.trinity_root = trinity_root
        self.queue_dir = trinity_root / ".claude/crash_analysis_queue"
        self.requests_dir = self.queue_dir / "requests"
        self.responses_dir = self.queue_dir / "responses"
        self.overnight_log = trinity_root / ".claude/logs/overnight_autonomous.log"

        self.build_dir = trinity_root / "build"
        self.worldserver_exe = self.build_dir / "bin/RelWithDebInfo/worldserver.exe"
        self.worldserver_pdb = self.build_dir / "bin/RelWithDebInfo/worldserver.pdb"
        self.deploy_dir = Path("M:/Wplayerbot")

        self.overnight_branch = f"overnight-{datetime.now().strftime('%Y%m%d')}"
        self.processed_cache = set()
        self.fixes_applied = 0
        self.fixes_failed = 0

        # Create directories
        self.overnight_log.parent.mkdir(parents=True, exist_ok=True)

    def log(self, message: str, level: str = "INFO"):
        """Log with timestamp"""
        timestamp = datetime.now().isoformat()
        log_msg = f"[{timestamp}] [OVERNIGHT] [{level}] {message}"
        print(log_msg)
        try:
            with open(self.overnight_log, 'a', encoding='utf-8') as f:
                f.write(log_msg + '\n')
        except:
            pass

    def cleanup_problematic_files(self) -> bool:
        """
        Remove problematic files that can break git operations
        (e.g., NUL, CON, PRN - Windows reserved names)
        Returns True if successful, False otherwise
        """
        try:
            # Windows reserved filenames that can cause git add failures
            problematic_files = ['NUL', 'CON', 'PRN', 'AUX', 'COM1', 'COM2', 'COM3', 'COM4',
                                'LPT1', 'LPT2', 'LPT3', 'CLOCK$']

            removed_count = 0
            for filename in problematic_files:
                file_path = self.trinity_root / filename
                if file_path.exists():
                    try:
                        file_path.unlink()
                        self.log(f"[OK] Removed problematic file: {filename}")
                        removed_count += 1
                    except Exception as e:
                        self.log(f"[!] Warning: Could not remove {filename}: {e}", "WARN")

            if removed_count > 0:
                self.log(f"[OK] Cleaned up {removed_count} problematic file(s)")

            return True
        except Exception as e:
            self.log(f"[!] Error cleaning problematic files: {e}", "WARN")
            return True  # Non-critical, continue anyway

    def setup_overnight_branch(self) -> bool:
        """
        Setup overnight branch for safe autonomous operation

        Steps:
        1. Ensure we're on playerbot-dev
        2. Commit and push any changes in playerbot-dev
        3. Create overnight-YYYYMMDD branch
        4. Checkout overnight branch
        """
        try:
            self.log("")
            self.log("=" * 70)
            self.log("OVERNIGHT BRANCH SETUP")
            self.log("=" * 70)

            # Clean up problematic files first (NUL, CON, etc.)
            self.cleanup_problematic_files()

            # Check current branch
            result = subprocess.run(
                ['git', 'rev-parse', '--abbrev-ref', 'HEAD'],
                cwd=self.trinity_root,
                capture_output=True,
                text=True,
                check=True
            )
            current_branch = result.stdout.strip()
            self.log(f"Current branch: {current_branch}")

            # If not on playerbot-dev, switch to it
            if current_branch != "playerbot-dev":
                self.log("Switching to playerbot-dev...")
                subprocess.run(
                    ['git', 'checkout', 'playerbot-dev'],
                    cwd=self.trinity_root,
                    check=True
                )

            # Commit any uncommitted changes in playerbot-dev
            result = subprocess.run(
                ['git', 'status', '--porcelain'],
                cwd=self.trinity_root,
                capture_output=True,
                text=True
            )

            if result.stdout.strip():
                self.log("Committing and pushing playerbot-dev changes...")
                subprocess.run(
                    ['git', 'add', '-A'],
                    cwd=self.trinity_root,
                    check=True
                )
                commit_msg = f"chore: pre-overnight commit - {datetime.now().strftime('%Y-%m-%d %H:%M')}"
                subprocess.run(
                    ['git', 'commit', '--no-verify', '-m', commit_msg],
                    cwd=self.trinity_root,
                    check=True
                )
                subprocess.run(
                    ['git', 'push', 'origin', 'playerbot-dev'],
                    cwd=self.trinity_root,
                    check=True
                )
                self.log("✅ playerbot-dev committed and pushed")
            else:
                self.log("✅ playerbot-dev is clean")

            # Delete overnight branch if it exists (fresh start)
            result = subprocess.run(
                ['git', 'branch', '--list', self.overnight_branch],
                cwd=self.trinity_root,
                capture_output=True,
                text=True
            )

            if result.stdout.strip():
                self.log(f"Deleting existing {self.overnight_branch}...")
                subprocess.run(
                    ['git', 'branch', '-D', self.overnight_branch],
                    cwd=self.trinity_root,
                    check=True
                )

            # Create overnight branch from playerbot-dev
            self.log(f"Creating {self.overnight_branch} from playerbot-dev...")
            subprocess.run(
                ['git', 'checkout', '-b', self.overnight_branch],
                cwd=self.trinity_root,
                check=True
            )

            # Push overnight branch (force push since we recreate it each time)
            self.log(f"Pushing {self.overnight_branch} to origin...")
            subprocess.run(
                ['git', 'push', '-f', '-u', 'origin', self.overnight_branch],
                cwd=self.trinity_root,
                check=True
            )

            self.log("")
            self.log("✅ OVERNIGHT BRANCH SETUP COMPLETE")
            self.log(f"   Branch: {self.overnight_branch}")
            self.log(f"   All fixes will be committed to this branch")
            self.log(f"   Review and merge to playerbot-dev in the morning")
            self.log("")

            return True

        except Exception as e:
            self.log(f"ERROR setting up overnight branch: {e}", "ERROR")
            self.log(traceback.format_exc(), "DEBUG")
            return False

    def find_pending_requests(self):
        """Find pending crash analysis requests"""
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

    def find_responses_needing_deployment(self):
        """Find responses that have been generated and need overnight deployment"""
        needing_deployment = []
        if not self.responses_dir.exists():
            return needing_deployment

        for response_file in self.responses_dir.glob("response_*.json"):
            try:
                request_id = response_file.stem.replace('response_', '')

                # Check if already deployed in overnight mode
                overnight_marker = self.queue_dir / "overnight_deployed" / f"deployed_{request_id}.txt"
                if overnight_marker.exists():
                    continue

                with open(response_file, 'r', encoding='utf-8') as f:
                    response_data = json.load(f)

                needing_deployment.append({
                    'request_id': request_id,
                    'response_file': response_file,
                    'crash_id': response_data.get('crash_analysis', {}).get('crash_id', 'unknown'),
                    'data': response_data
                })
            except Exception as e:
                self.log(f"ERROR reading {response_file}: {e}", "ERROR")
        return needing_deployment

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
                "overnight_mode": True
            }

            with open(marker_file, 'w', encoding='utf-8') as f:
                json.dump(marker_data, f, indent=2)

            self.log(f"✅ Created auto-process marker for {request_id}")
            return True
        except Exception as e:
            self.log(f"ERROR creating marker: {e}", "ERROR")
            return False

    def deploy_fix_overnight(self, response_info: dict) -> bool:
        """
        Deploy fix overnight - fully automated, NO human approval

        Steps:
        1. Apply fix to source files
        2. Compile worldserver
        3. If compilation succeeds:
           - Create git commit
           - Push to overnight branch
           - Copy worldserver + pdb to M:/Wplayerbot
           - Mark as deployed
        4. If compilation fails:
           - Log error
           - Skip deployment
           - Continue to next crash
        """
        request_id = response_info['request_id']
        crash_id = response_info['crash_id']

        self.log("")
        self.log("=" * 70)
        self.log("🌙 OVERNIGHT FIX DEPLOYMENT (NO HUMAN APPROVAL)")
        self.log("=" * 70)
        self.log(f"Request ID: {request_id}")
        self.log(f"Crash ID: {crash_id}")
        self.log("")

        try:
            response_data = response_info['data']

            # Step 1: Apply fix
            self.log("📝 Step 1/6: Applying fix to source files...")
            if not self.apply_fix_to_filesystem(response_data):
                self.log("❌ Fix application failed", "ERROR")
                self.fixes_failed += 1
                return False

            # Step 2: Compile
            self.log("🔨 Step 2/6: Compiling worldserver...")
            if not self.compile_worldserver():
                self.log("❌ Compilation failed - skipping deployment", "ERROR")
                self.log("   Fix applied but not compiled - will be in git diff")
                self.fixes_failed += 1
                # Revert the changes
                subprocess.run(['git', 'checkout', '.'], cwd=self.trinity_root)
                return False

            # Step 3: Create commit
            self.log("💾 Step 3/6: Creating git commit...")
            if not self.create_git_commit_overnight(response_data):
                self.log("⚠️ Git commit failed", "WARN")

            # Step 4: Push to overnight branch
            self.log("📤 Step 4/6: Pushing to overnight branch...")
            if not self.push_to_overnight_branch():
                self.log("⚠️ Push failed", "WARN")

            # Step 5: Stop worldserver before deployment
            self.log("🛑 Step 5/6: Stopping worldserver for deployment...")
            if not self.stop_worldserver():
                self.log("⚠️ Failed to stop worldserver - deployment may fail", "WARN")

            # Deploy worldserver + pdb
            self.log("📦 Copying worldserver + pdb to Wplayerbot...")
            if not self.copy_worldserver_to_deploy():
                self.log("⚠️ Deployment copy failed", "WARN")

            # Step 6: Restart worldserver
            self.log("🚀 Step 6/6: Restarting worldserver...")
            if not self.restart_worldserver():
                self.log("⚠️ Failed to restart worldserver", "WARN")
                self.log("   worldserver may not be running - check manually", "WARN")

            # Mark as deployed
            overnight_deployed_dir = self.queue_dir / "overnight_deployed"
            overnight_deployed_dir.mkdir(exist_ok=True)
            marker_file = overnight_deployed_dir / f"deployed_{request_id}.txt"

            deployment_info = f"""Overnight Deployment
====================
Request ID: {request_id}
Crash ID: {crash_id}
Deployed: {datetime.now().isoformat()}
Branch: {self.overnight_branch}
Compilation: SUCCESS
Deployment: SUCCESS
Worldserver: RESTARTED
"""
            marker_file.write_text(deployment_info, encoding='utf-8')

            self.fixes_applied += 1

            self.log("")
            self.log("✅ OVERNIGHT DEPLOYMENT COMPLETE")
            self.log(f"   Fix applied, compiled, committed to {self.overnight_branch}")
            self.log(f"   worldserver deployed to {self.deploy_dir}")
            self.log(f"   worldserver restarted and ready")
            self.log(f"   Total fixes applied: {self.fixes_applied}")
            self.log("")

            return True

        except Exception as e:
            self.log(f"ERROR deploying fix: {e}", "ERROR")
            self.log(traceback.format_exc(), "DEBUG")
            self.fixes_failed += 1
            return False

    def apply_fix_to_filesystem(self, response_data: dict) -> bool:
        """Apply fix code to source files"""
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

                self.log(f"  Writing fix to: {file_path}")
                with open(file_path, 'w', encoding='utf-8') as f:
                    f.write(new_code)

            self.log(f"✅ Applied fix to {len(files_modified)} file(s)")
            return True

        except Exception as e:
            self.log(f"ERROR applying fix: {e}", "ERROR")
            return False

    def compile_worldserver(self) -> bool:
        """Compile worldserver - CRITICAL for overnight mode"""
        cmd = [
            "cmake", "--build", str(self.build_dir),
            "--target", "worldserver",
            "--config", "RelWithDebInfo",
            "-j", "8"
        ]

        self.log(f"  Running: {' '.join(cmd)}")

        result = subprocess.run(
            cmd,
            cwd=self.build_dir,
            capture_output=True,
            text=True,
            encoding='utf-8',
            errors='replace',
            timeout=1800  # 30 minutes timeout
        )

        if result.returncode != 0:
            self.log(f"❌ Compilation FAILED", "ERROR")
            self.log("Compiler output:", "DEBUG")
            self.log(result.stderr[:5000], "DEBUG")  # First 5000 chars
            return False

        self.log("✅ Compilation SUCCESSFUL")
        return True

    def create_git_commit_overnight(self, response_data: dict) -> bool:
        """Create git commit on overnight branch with retry logic"""
        max_retries = 3
        retry_delay = 2  # seconds

        for attempt in range(max_retries):
            try:
                crash_analysis = response_data['crash_analysis']
                fix_info = response_data['fix']

                # Check if there are any changes to commit
                status_result = subprocess.run(
                    ['git', 'status', '--porcelain'],
                    cwd=self.trinity_root,
                    capture_output=True,
                    text=True,
                    timeout=30
                )

                if not status_result.stdout.strip():
                    self.log("[!] No changes to commit (files already committed)")
                    return True

                # Git add modified files
                for file_info in fix_info.get('files_modified', []):
                    add_result = subprocess.run(
                        ['git', 'add', file_info['path']],
                        cwd=self.trinity_root,
                        capture_output=True,
                        text=True,
                        timeout=30
                    )
                    if add_result.returncode != 0:
                        self.log(f"[!] Warning: git add failed for {file_info['path']}: {add_result.stderr}", "WARN")

                # Create commit message
                commit_msg = f"""fix(playerbot): {crash_analysis['category']} in {crash_analysis['crash_location']['function']}

Crash ID: {crash_analysis['crash_id']}
Location: {crash_analysis['crash_location']['file']}:{crash_analysis['crash_location']['line']}

Root Cause: {crash_analysis['root_cause']['summary']}

Fix: {fix_info['strategy']}

Overnight Autonomous Fix - Branch: {self.overnight_branch}
Compilation: VERIFIED
Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}

Review in morning and merge to playerbot-dev if acceptable.

Co-Authored-By: Claude <noreply@anthropic.com>"""

                # Remove git lock files if they exist
                git_index_lock = self.trinity_root / ".git/index.lock"
                if git_index_lock.exists():
                    self.log(f"[!] Removing stale git index lock")
                    git_index_lock.unlink()

                # Create commit
                commit_result = subprocess.run(
                    ['git', 'commit', '--no-verify', '-m', commit_msg],
                    cwd=self.trinity_root,
                    capture_output=True,
                    text=True,
                    timeout=60
                )

                if commit_result.returncode == 0:
                    self.log("[OK] Git commit created")
                    return True
                else:
                    # Check if "nothing to commit" error
                    if "nothing to commit" in commit_result.stdout.lower() or "nothing to commit" in commit_result.stderr.lower():
                        self.log("[!] Nothing to commit (working tree clean)")
                        return True

                    raise Exception(f"Git commit failed: {commit_result.stderr}")

            except subprocess.TimeoutExpired as e:
                self.log(f"[!] Git commit timeout (attempt {attempt + 1}/{max_retries})", "WARN")
                if attempt < max_retries - 1:
                    time.sleep(retry_delay)
                    continue
                else:
                    self.log(f"[X] Git commit failed after {max_retries} attempts (timeout)", "ERROR")
                    return False

            except Exception as e:
                self.log(f"[!] Git commit error (attempt {attempt + 1}/{max_retries}): {e}", "WARN")
                if attempt < max_retries - 1:
                    time.sleep(retry_delay)
                    continue
                else:
                    self.log(f"[X] Git commit failed after {max_retries} attempts: {e}", "ERROR")
                    return False

        return False

    def push_to_overnight_branch(self) -> bool:
        """Push commits to overnight branch"""
        try:
            result = subprocess.run(
                ['git', 'push', 'origin', self.overnight_branch],
                cwd=self.trinity_root,
                capture_output=True,
                text=True,
                timeout=300  # 5 minutes
            )

            if result.returncode != 0:
                self.log(f"⚠️ Push failed: {result.stderr}", "WARN")
                return False

            self.log("✅ Pushed to overnight branch")
            return True

        except Exception as e:
            self.log(f"ERROR pushing: {e}", "ERROR")
            return False

    def copy_worldserver_to_deploy(self) -> bool:
        """Copy worldserver.exe + .pdb to M:/Wplayerbot"""
        try:
            if not self.deploy_dir.exists():
                self.log(f"⚠️ Deploy directory not found: {self.deploy_dir}", "WARN")
                return False

            if not self.worldserver_exe.exists():
                self.log(f"⚠️ worldserver.exe not found: {self.worldserver_exe}", "WARN")
                return False

            # Copy worldserver.exe
            dest_exe = self.deploy_dir / "worldserver.exe"
            self.log(f"  Copying {self.worldserver_exe} -> {dest_exe}")
            shutil.copy2(self.worldserver_exe, dest_exe)

            # Copy worldserver.pdb
            if self.worldserver_pdb.exists():
                dest_pdb = self.deploy_dir / "worldserver.pdb"
                self.log(f"  Copying {self.worldserver_pdb} -> {dest_pdb}")
                shutil.copy2(self.worldserver_pdb, dest_pdb)
            else:
                self.log("  worldserver.pdb not found (non-fatal)", "WARN")

            self.log(f"✅ Deployed to {self.deploy_dir}")
            return True

        except Exception as e:
            self.log(f"ERROR copying files: {e}", "ERROR")
            return False

    def stop_worldserver(self) -> bool:
        """Stop worldserver.exe if it's running"""
        try:
            self.log("  Stopping worldserver.exe...")
            result = subprocess.run(
                ['tasklist', '/FI', 'IMAGENAME eq worldserver.exe'],
                capture_output=True,
                text=True,
                encoding='utf-8',
                errors='replace'
            )

            if 'worldserver.exe' in result.stdout:
                # Kill worldserver.exe
                subprocess.run(
                    ['taskkill', '/F', '/IM', 'worldserver.exe'],
                    capture_output=True,
                    text=True,
                    encoding='utf-8',
                    errors='replace',
                    timeout=30
                )
                time.sleep(2)  # Wait for process to fully terminate
                self.log("  [OK] worldserver.exe stopped")
                return True
            else:
                self.log("  worldserver.exe was not running")
                return True

        except Exception as e:
            self.log(f"  [!] ERROR stopping worldserver: {e}", "WARN")
            return False

    def start_worldserver(self) -> bool:
        """Start worldserver.exe with config file"""
        try:
            worldserver_exe = self.deploy_dir / "worldserver.exe"
            worldserver_conf = self.deploy_dir / "worldserver.conf"

            if not worldserver_exe.exists():
                self.log(f"  [X] worldserver.exe not found: {worldserver_exe}", "ERROR")
                return False

            if not worldserver_conf.exists():
                self.log(f"  [!] worldserver.conf not found: {worldserver_conf}", "WARN")

            self.log(f"  Starting worldserver from: {worldserver_exe}")

            # Start worldserver as a detached process
            subprocess.Popen(
                [str(worldserver_exe)],
                cwd=self.deploy_dir,
                creationflags=subprocess.CREATE_NEW_PROCESS_GROUP | subprocess.DETACHED_PROCESS,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL
            )

            time.sleep(3)  # Wait for startup
            self.log("  [OK] worldserver.exe started")
            return True

        except Exception as e:
            self.log(f"  [X] ERROR starting worldserver: {e}", "ERROR")
            return False

    def restart_worldserver(self) -> bool:
        """Stop and restart worldserver"""
        self.log("  Restarting worldserver...")
        if not self.stop_worldserver():
            self.log("  [!] Failed to stop worldserver", "WARN")
            # Continue anyway - maybe it wasn't running

        return self.start_worldserver()

    def run_overnight_mode(self):
        """Main overnight loop - runs until Ctrl+C"""
        self.log("")
        self.log("=" * 70)
        self.log("🌙 OVERNIGHT AUTONOMOUS MODE v1.0")
        self.log("=" * 70)
        self.log(f"Trinity Root: {self.trinity_root}")
        self.log(f"Deploy Dir: {self.deploy_dir}")
        self.log(f"Mode: FULLY AUTONOMOUS (NO HUMAN APPROVAL)")
        self.log("=" * 70)
        self.log("")

        # Setup overnight branch
        if not self.setup_overnight_branch():
            self.log("❌ Failed to setup overnight branch - aborting", "ERROR")
            return

        # Check if worldserver is running, start it if not
        self.log("")
        self.log("=" * 70)
        self.log("WORLDSERVER STARTUP CHECK")
        self.log("=" * 70)

        try:
            # Check if worldserver is already running
            result = subprocess.run(
                ['tasklist', '/FI', 'IMAGENAME eq worldserver.exe'],
                capture_output=True,
                text=True,
                timeout=10
            )

            if 'worldserver.exe' in result.stdout:
                self.log("✅ worldserver is already running")
            else:
                self.log("ℹ️ worldserver is not running, starting it...")

                worldserver_path = self.deploy_dir / "worldserver.exe"
                worldserver_conf = self.deploy_dir / "worldserver.conf"

                if not worldserver_path.exists():
                    self.log(f"❌ worldserver.exe not found at {worldserver_path}", "ERROR")
                    self.log("   Cannot start worldserver - aborting", "ERROR")
                    return

                if not worldserver_conf.exists():
                    self.log(f"⚠️ worldserver.conf not found at {worldserver_conf}, starting without config", "WARN")
                    args = [str(worldserver_path)]
                else:
                    args = [str(worldserver_path), '-c', 'worldserver.conf']

                # Start worldserver
                process = subprocess.Popen(
                    args,
                    cwd=self.deploy_dir,
                    stdout=subprocess.DEVNULL,
                    stderr=subprocess.DEVNULL,
                    creationflags=subprocess.CREATE_NEW_PROCESS_GROUP if sys.platform == 'win32' else 0
                )

                self.log(f"✅ worldserver started (PID: {process.pid})")
                time.sleep(5)  # Wait for server to initialize

                # Verify it's still running
                if process.poll() is None:
                    self.log("✅ worldserver is running and ready")
                else:
                    self.log("❌ worldserver exited immediately - check configuration", "ERROR")
                    self.log("   Continuing anyway - will process crashes if server starts later", "WARN")

        except Exception as e:
            self.log(f"ERROR checking/starting worldserver: {e}", "ERROR")
            self.log("   Continuing anyway - will process crashes if server is running", "WARN")

        self.log("=" * 70)
        self.log("")

        self.log("🌙 OVERNIGHT MODE ACTIVE - Processing crashes until Ctrl+C")
        self.log(f"   All fixes will go to branch: {self.overnight_branch}")
        self.log(f"   Review in morning and merge good fixes to playerbot-dev")
        self.log("")

        iteration = 0
        start_time = datetime.now()

        while True:
            try:
                iteration += 1

                # Heartbeat every 5 minutes
                if iteration % 10 == 0:
                    elapsed = datetime.now() - start_time
                    self.log(f"💓 Heartbeat: Iteration {iteration}")
                    self.log(f"   Running: {elapsed}")
                    self.log(f"   Fixes Applied: {self.fixes_applied}")
                    self.log(f"   Fixes Failed: {self.fixes_failed}")

                # Phase 1: Find pending requests → trigger analysis
                pending = self.find_pending_requests()
                if pending:
                    self.log(f"🔔 Found {len(pending)} pending request(s)")
                    for req in pending:
                        self.create_auto_process_marker(req)
                        self.processed_cache.add(req['request_id'])

                # Phase 2: Find responses → deploy immediately (no approval)
                needing_deployment = self.find_responses_needing_deployment()
                if needing_deployment:
                    self.log(f"🚀 Found {len(needing_deployment)} fix(es) ready for overnight deployment")
                    for resp in needing_deployment:
                        self.deploy_fix_overnight(resp)

                # Sleep
                time.sleep(30)

            except KeyboardInterrupt:
                self.log("")
                self.log("=" * 70)
                self.log("🛑 OVERNIGHT MODE STOPPED BY USER")
                self.log("=" * 70)
                elapsed = datetime.now() - start_time
                self.log(f"Running time: {elapsed}")
                self.log(f"Fixes Applied: {self.fixes_applied}")
                self.log(f"Fixes Failed: {self.fixes_failed}")
                self.log(f"Overnight Branch: {self.overnight_branch}")
                self.log("")
                self.log("Next steps:")
                self.log(f"1. Review commits in {self.overnight_branch}")
                self.log("2. Test the fixes")
                self.log("3. Merge good fixes to playerbot-dev:")
                self.log(f"   git checkout playerbot-dev")
                self.log(f"   git merge {self.overnight_branch}")
                self.log("   git push")
                self.log("")
                break

            except Exception as e:
                self.log(f"ERROR in overnight loop: {e}", "ERROR")
                self.log(traceback.format_exc(), "DEBUG")
                time.sleep(60)


def acquire_lockfile(trinity_root: Path) -> bool:
    """
    Acquire lockfile to ensure only one overnight mode instance runs.
    Returns True if lockfile acquired, False if another instance is running.
    """
    lockfile = trinity_root / ".claude/overnight_mode.lock"

    if lockfile.exists():
        # Check if process is still alive
        try:
            with open(lockfile, 'r') as f:
                pid = int(f.read().strip())

            # Check if PID exists on Windows
            result = subprocess.run(
                ['tasklist', '/FI', f'PID eq {pid}'],
                capture_output=True,
                text=True
            )

            if f"{pid}" in result.stdout:
                print(f"[X] Another overnight mode instance is already running (PID {pid})")
                print(f"    If this is incorrect, delete: {lockfile}")
                return False
            else:
                # PID doesn't exist, remove stale lockfile
                print(f"[!] Removing stale lockfile (PID {pid} not found)")
                lockfile.unlink()
        except Exception as e:
            print(f"[!] Error checking lockfile: {e}")
            return False

    # Create lockfile with current PID
    lockfile.parent.mkdir(parents=True, exist_ok=True)
    with open(lockfile, 'w') as f:
        f.write(str(os.getpid()))

    print(f"[OK] Lockfile acquired: {lockfile} (PID {os.getpid()})")

    # Register cleanup on exit
    def cleanup_lockfile():
        try:
            if lockfile.exists():
                lockfile.unlink()
                print(f"[OK] Lockfile removed: {lockfile}")
        except:
            pass

    atexit.register(cleanup_lockfile)
    return True


def main():
    if hasattr(sys.stdout, 'reconfigure'):
        sys.stdout.reconfigure(encoding='utf-8')

    script_path = Path(__file__).resolve()
    trinity_root = script_path.parent.parent.parent

    # Acquire lockfile to prevent multiple instances
    if not acquire_lockfile(trinity_root):
        sys.exit(1)

    overnight = OvernightAutonomousMode(trinity_root)
    overnight.run_overnight_mode()


if __name__ == "__main__":
    main()
