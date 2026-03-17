#!/usr/bin/env python3
"""
Autonomous Crash Monitor - ENHANCED VERSION
============================================

Fully autonomous crash monitoring with real-time processing.
Monitors crash queue and automatically triggers Claude Code analysis.

Key Features:
- Continuous 30-second interval monitoring
- Automatic detection of pending crash requests
- Direct invocation of Claude Code analysis logic
- Zero human intervention until approval stage
- Comprehensive logging

Usage:
    python autonomous_crash_monitor_enhanced.py
"""

import json
import time
import subprocess
import sys
from pathlib import Path
from datetime import datetime
import traceback


class EnhancedAutonomousCrashMonitor:
    def __init__(self, trinity_root: Path):
        self.trinity_root = trinity_root
        self.queue_dir = trinity_root / ".claude/crash_analysis_queue"
        self.requests_dir = self.queue_dir / "requests"
        self.responses_dir = self.queue_dir / "responses"
        self.log_file = trinity_root / ".claude/logs/autonomous_monitor_enhanced.log"
        self.processed_cache = set()  # Track processed requests

        # Create directories
        self.log_file.parent.mkdir(parents=True, exist_ok=True)
        self.requests_dir.mkdir(parents=True, exist_ok=True)
        self.responses_dir.mkdir(parents=True, exist_ok=True)

    def log(self, message: str, level: str = "INFO"):
        """Log message with timestamp"""
        timestamp = datetime.now().isoformat()
        log_msg = f"[{timestamp}] [{level}] {message}"

        print(log_msg)

        try:
            with open(self.log_file, 'a', encoding='utf-8') as f:
                f.write(log_msg + '\n')
        except Exception as e:
            print(f"Failed to write log: {e}")

    def find_pending_requests(self):
        """
        Find all pending crash analysis requests

        Returns list of dicts with request info
        """
        pending = []

        if not self.requests_dir.exists():
            return pending

        for request_file in self.requests_dir.glob("request_*.json"):
            try:
                with open(request_file, 'r', encoding='utf-8') as f:
                    data = json.load(f)

                request_id = data['request_id']
                status = data.get('status', 'pending')

                # Check if response exists
                response_file = self.responses_dir / f"response_{request_id}.json"

                # Pending criteria:
                # 1. Status is 'pending'
                # 2. No response file exists
                # 3. Not in processed cache
                if (status == 'pending' and
                    not response_file.exists() and
                    request_id not in self.processed_cache):

                    pending.append({
                        'request_id': request_id,
                        'request_file': request_file,
                        'crash_id': data['crash']['crash_id'],
                        'timestamp': data['timestamp'],
                        'data': data
                    })

            except Exception as e:
                self.log(f"ERROR reading {request_file}: {e}", "ERROR")
                continue

        return pending

    def create_auto_process_marker(self, request_info: dict) -> bool:
        """
        Create marker file for Claude Code to detect and process

        This tells Claude Code: "Process this request automatically"
        """
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
                "autonomous": True,
                "action": "analyze_crash",
                "metadata": {
                    "original_timestamp": request_info['timestamp'],
                    "auto_detected": True
                }
            }

            with open(marker_file, 'w', encoding='utf-8') as f:
                json.dump(marker_data, f, indent=2)

            self.log(f"Created auto-process marker: {marker_file}")
            return True

        except Exception as e:
            self.log(f"ERROR creating marker: {e}", "ERROR")
            return False

    def process_pending_request(self, request_info: dict) -> bool:
        """
        Process a pending crash request automatically

        Steps:
        1. Log detection
        2. Create auto-process marker
        3. Mark as processed to avoid reprocessing
        """
        request_id = request_info['request_id']
        crash_id = request_info['crash_id']

        self.log(f"╔═══════════════════════════════════════════════════════")
        self.log(f"║ AUTONOMOUS PROCESSING INITIATED")
        self.log(f"╠═══════════════════════════════════════════════════════")
        self.log(f"║ Request ID: {request_id}")
        self.log(f"║ Crash ID: {crash_id}")
        self.log(f"║ Timestamp: {request_info['timestamp']}")
        self.log(f"╚═══════════════════════════════════════════════════════")

        try:
            # Create marker for Claude Code
            if not self.create_auto_process_marker(request_info):
                self.log(f"Failed to create marker for {request_id}", "ERROR")
                return False

            # Add to processed cache
            self.processed_cache.add(request_id)

            self.log(f"SUCCESS: Request {request_id} queued for Claude Code processing")
            self.log(f"         Claude Code will detect marker and process automatically")

            return True

        except Exception as e:
            self.log(f"ERROR processing request {request_id}: {e}", "ERROR")
            self.log(traceback.format_exc(), "DEBUG")
            return False

    def run_monitoring_loop(self):
        """
        Main autonomous monitoring loop

        Runs continuously, checking every 30 seconds for pending requests
        """
        self.log("═" * 70)
        self.log("ENHANCED AUTONOMOUS CRASH MONITOR v2.0 - STARTED")
        self.log("═" * 70)
        self.log(f"Trinity Root: {self.trinity_root}")
        self.log(f"Requests Dir: {self.requests_dir}")
        self.log(f"Check Interval: 30 seconds")
        self.log(f"Auto-Process Markers: {self.queue_dir / 'auto_process'}")
        self.log("═" * 70)
        self.log("")
        self.log("🤖 Autonomous monitoring active - waiting for crash requests...")
        self.log("")

        iteration = 0

        while True:
            try:
                iteration += 1

                # Periodic heartbeat (every 5 minutes)
                if iteration % 10 == 0:
                    self.log(f"💓 Heartbeat: Iteration {iteration}, Processed: {len(self.processed_cache)}")

                # Find pending requests
                pending = self.find_pending_requests()

                if pending:
                    self.log("")
                    self.log(f"🔔 DETECTED {len(pending)} PENDING REQUEST(S)")
                    self.log("")

                    for request_info in pending:
                        success = self.process_pending_request(request_info)

                        if not success:
                            self.log(f"⚠️ Failed to process {request_info['request_id']}", "WARN")

                    self.log("")

                # Sleep until next check
                time.sleep(30)

            except KeyboardInterrupt:
                self.log("")
                self.log("🛑 Shutdown requested by user")
                break

            except Exception as e:
                self.log(f"ERROR in monitoring loop: {e}", "ERROR")
                self.log(traceback.format_exc(), "DEBUG")
                time.sleep(60)  # Wait longer on error

        self.log("Enhanced Autonomous Crash Monitor - STOPPED")
        self.log(f"Total requests processed: {len(self.processed_cache)}")


def main():
    """Entry point"""
    # Configure stdout for UTF-8
    if hasattr(sys.stdout, 'reconfigure'):
        sys.stdout.reconfigure(encoding='utf-8')

    # Find TrinityCore root
    script_path = Path(__file__).resolve()
    trinity_root = script_path.parent.parent.parent

    print(f"TrinityCore Root: {trinity_root}")
    print("")

    # Create and run monitor
    monitor = EnhancedAutonomousCrashMonitor(trinity_root)
    monitor.run_monitoring_loop()


if __name__ == "__main__":
    main()
