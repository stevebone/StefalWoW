#!/usr/bin/env python3
"""
Enterprise Automated Crash Loop - HYBRID SYSTEM
Python Orchestrator + Claude Code Comprehensive Analysis

Architecture:
- Python: Crash detection, monitoring, orchestration, compilation
- Claude Code: Comprehensive analysis using ALL resources (MCP, Serena, agents)
- Communication: File-based JSON protocol

Usage:
    python crash_auto_fix_hybrid.py --auto-run
    python crash_auto_fix_hybrid.py --single-iteration

In another terminal/Claude Code:
    Monitor for requests and process with: /analyze-crash
"""

import sys
import os
import json
import time
import subprocess
import argparse
import uuid
from pathlib import Path
from datetime import datetime
from typing import Dict, List, Optional, Tuple
import importlib.util

# Add scripts directory to path
scripts_dir = Path(__file__).parent
sys.path.insert(0, str(scripts_dir))

# Import our crash analysis modules
from crash_analyzer import CrashAnalyzer, CrashInfo
from crash_monitor import RealTimeCrashMonitor, LogMonitor, CrashContextAnalyzer

# ============================================================================
# File-Based Communication Protocol
# ============================================================================

class ClaudeCodeCommunicator:
    """
    Manages communication between Python and Claude Code via file-based protocol
    """

    def __init__(self, trinity_root: Path):
        self.trinity_root = Path(trinity_root)
        self.queue_dir = trinity_root / ".claude" / "crash_analysis_queue"
        self.queue_dir.mkdir(parents=True, exist_ok=True)

        # Subdirectories for organization
        self.requests_dir = self.queue_dir / "requests"
        self.responses_dir = self.queue_dir / "responses"
        self.in_progress_dir = self.queue_dir / "in_progress"
        self.completed_dir = self.queue_dir / "completed"

        for dir in [self.requests_dir, self.responses_dir,
                    self.in_progress_dir, self.completed_dir]:
            dir.mkdir(parents=True, exist_ok=True)

    def submit_analysis_request(self, crash: CrashInfo, context: dict) -> str:
        """
        Submit crash analysis request to Claude Code

        Returns: request_id
        """
        request_id = str(uuid.uuid4())[:8]

        request_data = {
            "request_id": request_id,
            "timestamp": datetime.now().isoformat(),
            "status": "pending",
            "crash": {
                "crash_id": crash.crash_id,
                "timestamp": crash.timestamp,
                "exception_code": crash.exception_code,
                "crash_location": crash.crash_location,
                "crash_function": crash.crash_function,
                "error_message": crash.error_message,
                "call_stack": crash.call_stack,
                "crash_category": crash.crash_category,
                "severity": crash.severity,
                "is_bot_related": crash.is_bot_related,
                "root_cause_hypothesis": crash.root_cause_hypothesis,
                "fix_suggestions": crash.fix_suggestions
            },
            "context": {
                "errors_before_crash": [
                    {"message": e.message, "timestamp": e.timestamp.isoformat()}
                    for e in context.get("errors_before_crash", [])
                ],
                "warnings_before_crash": [
                    {"message": w.message, "timestamp": w.timestamp.isoformat()}
                    for w in context.get("warnings_before_crash", [])
                ],
                "server_log_context": context.get("server_log_context", []),
                "playerbot_log_context": context.get("playerbot_log_context", [])
            },
            "request_type": "comprehensive_analysis",
            "priority": "high" if crash.severity == "CRITICAL" else "medium"
        }

        request_file = self.requests_dir / f"request_{request_id}.json"
        with open(request_file, 'w', encoding='utf-8') as f:
            json.dump(request_data, f, indent=2)

        print(f"\n📤 Submitted analysis request: {request_id}")
        print(f"   File: {request_file}")
        print(f"   Status: Waiting for Claude Code to process...")

        return request_id

    def wait_for_response(self, request_id: str, timeout: int = 600,
                         poll_interval: int = 5) -> Optional[Dict]:
        """
        Wait for Claude Code to process request and return response

        Args:
            request_id: Request ID to wait for
            timeout: Maximum wait time in seconds (default: 10 minutes)
            poll_interval: How often to check for response (default: 5 seconds)

        Returns:
            Response data dict or None if timeout
        """
        response_file = self.responses_dir / f"response_{request_id}.json"
        request_file = self.requests_dir / f"request_{request_id}.json"

        print(f"\n⏳ Waiting for Claude Code analysis...")
        print(f"   Request ID: {request_id}")
        print(f"   Timeout: {timeout}s")
        print(f"   Polling every: {poll_interval}s")

        start_time = time.time()
        dots = 0

        while time.time() - start_time < timeout:
            # Check if response exists
            if response_file.exists():
                print(f"\n✅ Response received!")

                with open(response_file, 'r', encoding='utf-8') as f:
                    response_data = json.load(f)

                # Move request to completed
                if request_file.exists():
                    completed_file = self.completed_dir / f"request_{request_id}.json"
                    request_file.rename(completed_file)

                # Move response to completed
                completed_response = self.completed_dir / f"response_{request_id}.json"
                response_file.rename(completed_response)

                return response_data

            # Show progress
            dots = (dots + 1) % 4
            print(f"\r   Waiting{'.' * dots}{' ' * (3 - dots)}", end='', flush=True)

            time.sleep(poll_interval)

        print(f"\n\n⏱️  Timeout waiting for response ({timeout}s)")
        print(f"   Request may still be processing in Claude Code")
        return None

    def get_pending_requests(self) -> List[Dict]:
        """Get all pending analysis requests (for Claude Code to process)"""
        pending = []
        for request_file in self.requests_dir.glob("request_*.json"):
            with open(request_file, 'r', encoding='utf-8') as f:
                data = json.load(f)
                if data.get("status") == "pending":
                    pending.append(data)
        return pending

    def mark_request_in_progress(self, request_id: str):
        """Mark request as being processed by Claude Code"""
        request_file = self.requests_dir / f"request_{request_id}.json"
        if request_file.exists():
            in_progress_file = self.in_progress_dir / f"request_{request_id}.json"
            request_file.rename(in_progress_file)

            # Update status
            with open(in_progress_file, 'r', encoding='utf-8') as f:
                data = json.load(f)
            data["status"] = "in_progress"
            data["processing_started"] = datetime.now().isoformat()
            with open(in_progress_file, 'w', encoding='utf-8') as f:
                json.dump(data, f, indent=2)

# ============================================================================
# Hybrid Fix Generator (Python Orchestrator)
# ============================================================================

class HybridFixGenerator:
    """
    Hybrid fix generator that delegates comprehensive analysis to Claude Code
    """

    def __init__(self, trinity_root: Path):
        self.trinity_root = Path(trinity_root)
        self.fixes_dir = trinity_root / ".claude" / "automated_fixes"
        self.fixes_dir.mkdir(parents=True, exist_ok=True)
        self.communicator = ClaudeCodeCommunicator(trinity_root)

    def generate_fix(self, crash: CrashInfo, context: dict,
                    timeout: int = 600) -> Optional[Path]:
        """
        Generate fix using hybrid approach:
        1. Python submits request
        2. Claude Code does comprehensive analysis
        3. Python receives and saves fix

        Args:
            crash: Crash information
            context: Log context
            timeout: How long to wait for Claude Code (default: 10 minutes)

        Returns:
            Path to generated fix file, or None if timeout/error
        """
        print(f"\n🔧 Generating HYBRID fix for: {crash.crash_id}")
        print(f"   Mode: Python Orchestrator + Claude Code Analysis")

        # Submit request to Claude Code
        request_id = self.communicator.submit_analysis_request(crash, context)

        print(f"\n📋 MANUAL STEP REQUIRED:")
        print(f"   Open Claude Code and run: /analyze-crash {request_id}")
        print(f"   Or Claude Code will auto-detect if monitoring is enabled")

        # Wait for response
        response = self.communicator.wait_for_response(request_id, timeout=timeout)

        if not response:
            print(f"\n❌ No response from Claude Code within timeout")
            print(f"   You can manually check for response later:")
            print(f"   Response file: .claude/crash_analysis_queue/responses/response_{request_id}.json")
            return None

        # Extract fix from response
        fix_content = response.get("fix_content", "")
        if not fix_content:
            print(f"\n⚠️  Claude Code returned no fix content")
            return None

        # Save fix to file
        fix_file = self.fixes_dir / f"fix_hybrid_{crash.crash_id}_{datetime.now().strftime('%Y%m%d_%H%M%S')}.cpp"

        with open(fix_file, 'w', encoding='utf-8') as f:
            f.write(fix_content)

        print(f"\n✅ Hybrid fix generated: {fix_file.name}")

        # Print analysis summary
        self._print_analysis_summary(response)

        return fix_file

    def _print_analysis_summary(self, response: Dict):
        """Print summary of Claude Code analysis"""
        print(f"\n" + "="*80)
        print(f"📊 CLAUDE CODE ANALYSIS SUMMARY")
        print(f"="*80)

        trinity_api = response.get("trinity_api_analysis", {})
        if trinity_api:
            print(f"\n🎮 TrinityCore API Analysis:")
            print(f"   Query Type: {trinity_api.get('query_type', 'N/A')}")
            print(f"   Relevant APIs: {len(trinity_api.get('relevant_apis', []))}")
            print(f"   Recommended: {trinity_api.get('recommended_approach', 'N/A')}")

        playerbot = response.get("playerbot_feature_analysis", {})
        if playerbot:
            print(f"\n🤖 Playerbot Feature Analysis:")
            print(f"   Existing Patterns: {len(playerbot.get('existing_patterns', []))}")
            print(f"   Available APIs: {len(playerbot.get('available_apis', []))}")
            print(f"   Integration Points: {len(playerbot.get('integration_points', []))}")

        strategy = response.get("fix_strategy", {})
        if strategy:
            print(f"\n🎯 Fix Strategy:")
            print(f"   Approach: {strategy.get('approach', 'N/A')}")
            print(f"   Hierarchy Level: {strategy.get('hierarchy_level', 'N/A')}")
            print(f"   Core Files Modified: {strategy.get('core_files_modified', 0)}")
            print(f"   Module Files Modified: {strategy.get('module_files_modified', 0)}")
            print(f"   Stability: {strategy.get('long_term_stability', 'N/A')}")

        validation = response.get("validation", {})
        if validation:
            print(f"\n✅ Quality Validation:")
            print(f"   Valid: {validation.get('valid', False)}")
            print(f"   Quality Score: {validation.get('quality_score', 'N/A')}")
            print(f"   Confidence: {validation.get('confidence', 'N/A')}")

        print(f"\n" + "="*80 + "\n")

# ============================================================================
# Hybrid Crash Loop Orchestrator
# ============================================================================

class HybridCrashLoopOrchestrator:
    """
    Orchestrates crash loop using hybrid approach
    """

    def __init__(
        self,
        trinity_root: Path,
        server_exe: Path,
        server_log: Path,
        playerbot_log: Path,
        crashes_dir: Path
    ):
        self.trinity_root = Path(trinity_root)
        self.server_exe = Path(server_exe)
        self.crashes_dir = Path(crashes_dir)

        # Create analyzers
        # Pass PDB directory for symbol resolution
        pdb_dir = server_exe.parent  # PDB files usually in same directory as executable
        self.crash_analyzer = CrashAnalyzer(trinity_root, crashes_dir, pdb_dir=pdb_dir)
        self.fix_generator = HybridFixGenerator(trinity_root)

        # Create log monitors
        self.server_monitor = LogMonitor(server_log, "Server.log", buffer_size=500)
        self.playerbot_monitor = LogMonitor(playerbot_log, "Playerbot.log", buffer_size=500)
        self.context_analyzer = CrashContextAnalyzer(
            self.server_monitor,
            self.playerbot_monitor,
            crashes_dir
        )

        # State tracking
        self.iteration = 0
        self.max_iterations = 10
        self.server_timeout = 300  # Default: 5 minutes
        self.crash_history: List[str] = []
        self.fixes_applied: List[Path] = []

    def run_crash_loop(self, auto_fix: bool = False, auto_compile: bool = False,
                      analysis_timeout: int = 600):
        """
        Run hybrid crash loop

        Args:
            auto_fix: Automatically apply fixes (default: False)
            auto_compile: Automatically compile (default: False)
            analysis_timeout: How long to wait for Claude Code analysis (default: 10 min)
        """
        print("\n" + "="*100)
        print("🚀 HYBRID CRASH LOOP - Python Orchestrator + Claude Code Analysis")
        print("="*100)
        print(f"\n⚙️  Configuration:")
        print(f"   Trinity Root: {self.trinity_root}")
        print(f"   Server Exe: {self.server_exe}")
        print(f"   Max Iterations: {self.max_iterations}")
        print(f"   Auto-Fix: {auto_fix}")
        print(f"   Auto-Compile: {auto_compile}")
        print(f"   Server Timeout: {self.server_timeout}s ({self.server_timeout/60:.1f} min)" if self.server_timeout > 0 else "   Server Timeout: Disabled (runs until crash)")
        print(f"   Analysis Timeout: {analysis_timeout}s ({analysis_timeout/60:.1f} min)")
        print(f"\n💡 HYBRID MODE:")
        print(f"   - Python: Crash detection, monitoring, compilation, orchestration")
        print(f"   - Claude Code: Comprehensive analysis (MCP, Serena, agents)")
        print(f"   - Communication: File-based JSON protocol")
        print()

        # Start log monitors
        print("📊 Starting log monitors...")
        self.server_monitor.start()
        self.playerbot_monitor.start()
        time.sleep(2)
        print("✅ Log monitors started\n")

        # Main crash loop
        while self.iteration < self.max_iterations:
            self.iteration += 1
            print("\n" + "="*100)
            print(f"🔁 ITERATION {self.iteration}/{self.max_iterations}")
            print("="*100 + "\n")

            # Step 1: Run server
            print("🚀 Step 1: Running server...")
            crashed, crash_file = self._run_server(timeout=self.server_timeout)

            if not crashed:
                print("\n✅ SUCCESS! Server ran without crashing!")
                print(f"✅ System is STABLE after {self.iteration} iterations")
                break

            print(f"\n💥 Server crashed! Analyzing...")

            # Step 2: Python does basic analysis
            print(f"\n🔍 Step 2: Python basic analysis...")
            crash_info = self.crash_analyzer.parse_crash_dump(crash_file)

            if not crash_info:
                print("❌ Failed to parse crash dump, skipping iteration")
                continue

            # Get log context
            context_info = self.context_analyzer.analyze_crash_with_context(crash_file)

            # Print basic report
            self.crash_analyzer.print_crash_report(crash_info)

            # Check for repeated crash
            if crash_info.crash_id in self.crash_history:
                repeat_count = self.crash_history.count(crash_info.crash_id) + 1
                print(f"\n⚠️  WARNING: Same crash repeated ({repeat_count} times)")

                if repeat_count >= 3:
                    print("\n❌ CRASH LOOP DETECTED!")
                    print("   Same crash occurred 3+ times despite fixes.")
                    print("   Manual intervention required.\n")
                    break

            self.crash_history.append(crash_info.crash_id)

            # Step 3: Delegate to Claude Code for comprehensive analysis
            print(f"\n🔧 Step 3: Delegating to Claude Code for comprehensive analysis...")
            print(f"   This will use ALL Claude Code resources:")
            print(f"   - Trinity MCP Server (TrinityCore API research)")
            print(f"   - Serena MCP (Playerbot codebase analysis)")
            print(f"   - Specialized agents (trinity-researcher, etc.)")

            # Convert LogEntry objects to strings for JSON serialization
            context_dict = {
                "errors_before_crash": [entry.raw_line for entry in context_info.errors_before_crash],
                "warnings_before_crash": [entry.raw_line for entry in context_info.warnings_before_crash],
                "server_log_context": [entry.raw_line for entry in context_info.server_log_pre_crash[:30]],
                "playerbot_log_context": [entry.raw_line for entry in context_info.playerbot_log_pre_crash[:30]]
            }

            fix_file = self.fix_generator.generate_fix(
                crash_info, context_dict, timeout=analysis_timeout
            )

            if not fix_file:
                print("   ⚠️  No fix received from Claude Code")
                continue

            self.fixes_applied.append(fix_file)

            # Step 4: Apply fix (if auto_fix enabled)
            if auto_fix:
                print(f"\n🛠️  Step 4: Applying fix automatically...")
                print(f"   Fix file: {fix_file}")
            else:
                print(f"\n📋 Step 4: Fix generated (manual application required)")
                print(f"   Fix file: {fix_file}")
                print(f"   Review and apply the fix manually, then continue.\n")

                response = input("   Apply fix and continue? (y/n): ").strip().lower()
                if response != 'y':
                    print("   Stopping crash loop.")
                    break

            # Step 5: Compile
            if auto_compile and auto_fix:
                print(f"\n🔨 Step 5: Compiling with changes...")
                if self._compile_server():
                    print("   ✅ Compilation successful")
                else:
                    print("   ❌ Compilation failed, stopping loop")
                    break
            else:
                print(f"\n📝 Step 5: Manual compilation required")
                print("   Compile the server and press Enter to continue...")
                input()

            print(f"\n✅ Iteration {self.iteration} complete, starting next test run...\n")
            time.sleep(2)

        # Loop finished
        self._print_final_summary()

        # Stop monitors
        self.server_monitor.stop()
        self.playerbot_monitor.stop()

    def _run_server(self, timeout: int = 300) -> Tuple[bool, Optional[Path]]:
        """Run server and detect crash"""
        print(f"   Executable: {self.server_exe}")
        if timeout > 0:
            print(f"   Timeout: {timeout}s ({timeout/60:.1f} min)")
        else:
            print(f"   Timeout: Disabled (will run until crash)")

        crashes_before = set(self.crashes_dir.glob("*worldserver*.txt"))

        try:
            process = subprocess.Popen(
                [str(self.server_exe)],
                cwd=self.server_exe.parent,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE
            )

            print(f"   Process ID: {process.pid}")

            try:
                # If timeout is 0, wait indefinitely (None)
                wait_timeout = timeout if timeout > 0 else None
                returncode = process.wait(timeout=wait_timeout)
            except subprocess.TimeoutExpired:
                print(f"\n   ✅ Server ran for {timeout}s without crashing")
                process.terminate()
                process.wait(timeout=10)
                return False, None

            if returncode != 0:
                print(f"\n   💥 Server exited with code {returncode}")
                time.sleep(2)

                crashes_after = set(self.crashes_dir.glob("*worldserver*.txt"))
                new_crashes = crashes_after - crashes_before

                if new_crashes:
                    crash_file = sorted(new_crashes, key=lambda p: p.stat().st_mtime)[-1]
                    return True, crash_file
                else:
                    print("   ⚠️  No crash dump found")
                    return True, None
            else:
                return False, None

        except Exception as e:
            print(f"   ❌ Error running server: {e}")
            return False, None

    def _compile_server(self) -> bool:
        """Compile server with CMake"""
        build_dir = self.trinity_root / "build"

        try:
            print(f"   Build directory: {build_dir}")
            result = subprocess.run(
                ["cmake", "--build", ".", "--target", "worldserver", "--config", "RelWithDebInfo"],
                cwd=build_dir,
                capture_output=True,
                text=True,
                timeout=1800
            )
            return result.returncode == 0
        except Exception as e:
            print(f"   ❌ Build error: {e}")
            return False

    def _print_final_summary(self):
        """Print final summary"""
        print("\n" + "="*100)
        print("📊 HYBRID CRASH LOOP SUMMARY")
        print("="*100)
        print(f"\n   Total Iterations: {self.iteration}")
        print(f"   Unique Crashes: {len(set(self.crash_history))}")
        print(f"   Total Crashes: {len(self.crash_history)}")
        print(f"   Fixes Generated: {len(self.fixes_applied)}")

        if self.crash_history:
            print(f"\n   Crash Breakdown:")
            from collections import Counter
            for crash_id, count in Counter(self.crash_history).most_common():
                print(f"      - {crash_id} ({count}x)")

        print("\n   ✅ Hybrid mode: Python orchestration + Claude Code analysis")
        print("   ✅ Comprehensive analysis using ALL Claude Code resources")
        print("\n" + "="*100 + "\n")

# ============================================================================
# Main Entry Point
# ============================================================================

def main():
    parser = argparse.ArgumentParser(
        description="Hybrid Crash Loop - Python Orchestrator + Claude Code Analysis"
    )

    parser.add_argument(
        '--auto-run',
        action='store_true',
        help='Run fully automated loop'
    )

    parser.add_argument(
        '--single-iteration',
        action='store_true',
        help='Run single iteration'
    )

    parser.add_argument(
        '--trinity-root',
        type=Path,
        default=Path('c:/TrinityBots/TrinityCore'),
        help='TrinityCore root directory'
    )

    parser.add_argument(
        '--server-exe',
        type=Path,
        default=Path('M:/Wplayerbot/worldserver.exe'),
        help='Server executable path'
    )

    parser.add_argument(
        '--server-log',
        type=Path,
        default=Path('M:/Wplayerbot/Logs/Server.log'),
        help='Server.log path'
    )

    parser.add_argument(
        '--playerbot-log',
        type=Path,
        default=Path('M:/Wplayerbot/Logs/Playerbot.log'),
        help='Playerbot.log path'
    )

    parser.add_argument(
        '--crashes',
        type=Path,
        default=Path('M:/Wplayerbot/Crashes'),
        help='Crash dumps directory'
    )

    parser.add_argument(
        '--max-iterations',
        type=int,
        default=10,
        help='Maximum crash loop iterations'
    )

    parser.add_argument(
        '--analysis-timeout',
        type=int,
        default=600,
        help='Timeout for Claude Code analysis (seconds, default: 600 = 10 min)'
    )

    parser.add_argument(
        '--server-timeout',
        type=int,
        default=300,
        help='Timeout for server execution before considering it stable (seconds, default: 300 = 5 min, 0 = no timeout)'
    )

    args = parser.parse_args()

    print("""
╔══════════════════════════════════════════════════════════════════════════╗
║                                                                          ║
║                    HYBRID CRASH AUTOMATION SYSTEM                        ║
║                                                                          ║
║  Python Orchestrator + Claude Code Comprehensive Analysis               ║
║                                                                          ║
║  Python handles:                                                         ║
║  - Crash detection and monitoring                                       ║
║  - Server execution and compilation                                     ║
║  - Loop orchestration                                                    ║
║                                                                          ║
║  Claude Code handles:                                                    ║
║  - Trinity MCP Server (TrinityCore API research)                        ║
║  - Serena MCP (Playerbot codebase analysis)                             ║
║  - Specialized agents (trinity-researcher, code-quality-reviewer, etc.) ║
║  - Comprehensive fix generation                                          ║
║                                                                          ║
║  Communication: File-based JSON protocol                                 ║
║                                                                          ║
╚══════════════════════════════════════════════════════════════════════════╝
""")

    # Create orchestrator
    orchestrator = HybridCrashLoopOrchestrator(
        trinity_root=args.trinity_root,
        server_exe=args.server_exe,
        server_log=args.server_log,
        playerbot_log=args.playerbot_log,
        crashes_dir=args.crashes
    )

    orchestrator.max_iterations = args.max_iterations
    orchestrator.server_timeout = args.server_timeout

    # Run based on mode
    if args.auto_run:
        print("🤖 Running in FULLY AUTOMATED mode")
        print("   ⚠️  This will automatically apply fixes and recompile!\n")
        response = input("   Continue? (y/n): ").strip().lower()
        if response == 'y':
            orchestrator.run_crash_loop(
                auto_fix=True,
                auto_compile=True,
                analysis_timeout=args.analysis_timeout
            )
    elif args.single_iteration:
        print("👤 Running in MANUAL mode (single iteration)\n")
        orchestrator.max_iterations = 1
        orchestrator.run_crash_loop(
            auto_fix=False,
            auto_compile=False,
            analysis_timeout=args.analysis_timeout
        )
    else:
        print("👤 Running in SEMI-AUTOMATED mode (manual review required)\n")
        orchestrator.run_crash_loop(
            auto_fix=False,
            auto_compile=False,
            analysis_timeout=args.analysis_timeout
        )

if __name__ == "__main__":
    main()
