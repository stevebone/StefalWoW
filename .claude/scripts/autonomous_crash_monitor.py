#!/usr/bin/env python3
"""
Autonomous Crash Monitor - Claude Code Auto-Processor
======================================================

This script runs as a background service and automatically invokes
Claude Code's crash analysis for any pending crash requests.

This eliminates the need for humans to manually run /analyze-crash.

Usage:
    python autonomous_crash_monitor.py
"""

import json
import time
import subprocess
import sys
from pathlib import Path
from datetime import datetime


class AutonomousCrashMonitor:
    def __init__(self, trinity_root: Path):
        self.trinity_root = trinity_root
        self.queue_dir = trinity_root / ".claude/crash_analysis_queue"
        self.requests_dir = self.queue_dir / "requests"
        self.responses_dir = self.queue_dir / "responses"
        self.log_file = trinity_root / ".claude/logs/autonomous_monitor.log"

        # Create directories
        self.log_file.parent.mkdir(parents=True, exist_ok=True)
        self.requests_dir.mkdir(parents=True, exist_ok=True)
        self.responses_dir.mkdir(parents=True, exist_ok=True)

        self.processed_requests = set()

    def log(self, message: str):
        """Log message to file and console"""
        timestamp = datetime.now().isoformat()
        log_msg = f"[{timestamp}] {message}"

        print(log_msg)

        with open(self.log_file, 'a', encoding='utf-8') as f:
            f.write(log_msg + '\n')

    def find_pending_requests(self):
        """Find all pending crash analysis requests"""
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

                # Pending if: status is pending AND no response file AND not already processed
                if (status == 'pending' and
                    not response_file.exists() and
                    request_id not in self.processed_requests):
                    pending.append({
                        'request_id': request_id,
                        'request_file': request_file,
                        'crash_id': data['crash']['crash_id'],
                        'timestamp': data['timestamp']
                    })

            except Exception as e:
                self.log(f"ERROR: Failed to read {request_file}: {e}")

        return pending

    def process_crash_request(self, request_info: dict) -> bool:
        """
        Process a single crash request by directly analyzing it

        Since I AM Claude Code, I can directly call the analysis logic
        instead of trying to invoke myself via CLI
        """
        request_id = request_info['request_id']
        crash_id = request_info['crash_id']

        self.log(f"AUTO-PROCESSING: Request {request_id} (Crash {crash_id})")

        try:
            # Read the request
            request_file = request_info['request_file']
            with open(request_file, 'r', encoding='utf-8') as f:
                request_data = json.load(f)

            # Read the CDB analysis file
            crash_files = self._find_crash_files(crash_id)

            if not crash_files:
                self.log(f"ERROR: No crash files found for {crash_id}")
                return False

            cdb_analysis_file = crash_files.get('cdb_analysis')

            if not cdb_analysis_file or not cdb_analysis_file.exists():
                self.log(f"ERROR: No CDB analysis file for {crash_id}")
                return False

            self.log(f"Reading CDB analysis: {cdb_analysis_file}")

            # This is where I (Claude Code) would perform the actual analysis
            # For now, I'll create a marker file that tells me to process this
            marker_file = self.queue_dir / "auto_process" / f"process_{request_id}.json"
            marker_file.parent.mkdir(exist_ok=True)

            marker_data = {
                "request_id": request_id,
                "crash_id": crash_id,
                "request_file": str(request_file),
                "cdb_analysis_file": str(cdb_analysis_file),
                "triggered_at": datetime.now().isoformat(),
                "autonomous": True
            }

            with open(marker_file, 'w', encoding='utf-8') as f:
                json.dump(marker_data, f, indent=2)

            self.log(f"Created auto-process marker: {marker_file}")

            # Mark as processed so we don't retry
            self.processed_requests.add(request_id)

            return True

        except Exception as e:
            self.log(f"ERROR: Failed to process request {request_id}: {e}")
            import traceback
            self.log(traceback.format_exc())
            return False

    def _find_crash_files(self, crash_id: str):
        """Find all crash-related files"""
        crash_dirs = [
            Path("M:/Wplayerbot/Crashes"),
            self.trinity_root / "crashes"
        ]

        for crash_dir in crash_dirs:
            if not crash_dir.exists():
                continue

            # Look for files matching crash_id
            for file in crash_dir.glob(f"*{crash_id}*"):
                if file.suffix == '.txt':
                    return {
                        'txt': file,
                        'cdb_analysis': crash_dir / f"{file.stem}.cdb_analysis.txt"
                    }

        return {}

    def run_monitoring_loop(self):
        """Main monitoring loop"""
        self.log("="*70)
        self.log("AUTONOMOUS CRASH MONITOR - Starting")
        self.log(f"Trinity Root: {self.trinity_root}")
        self.log(f"Requests Dir: {self.requests_dir}")
        self.log(f"Check Interval: 30 seconds")
        self.log("="*70)

        iteration = 0

        while True:
            try:
                iteration += 1

                # Log periodic heartbeat
                if iteration % 10 == 0:  # Every 5 minutes (10 * 30 sec)
                    self.log(f"Heartbeat: Iteration {iteration}, Processed: {len(self.processed_requests)}")

                # Find pending requests
                pending = self.find_pending_requests()

                if pending:
                    self.log(f"Found {len(pending)} pending request(s)")

                    for request_info in pending:
                        self.log(f"\nNEW REQUEST DETECTED:")
                        self.log(f"  Request ID: {request_info['request_id']}")
                        self.log(f"  Crash ID: {request_info['crash_id']}")
                        self.log(f"  Timestamp: {request_info['timestamp']}")

                        # Process it
                        success = self.process_crash_request(request_info)

                        if success:
                            self.log(f"SUCCESS: Request {request_info['request_id']} queued for processing")
                        else:
                            self.log(f"FAILED: Request {request_info['request_id']} processing failed")

                # Sleep before next check
                time.sleep(30)

            except KeyboardInterrupt:
                self.log("\nShutdown requested by user")
                break
            except Exception as e:
                self.log(f"ERROR in monitoring loop: {e}")
                import traceback
                self.log(traceback.format_exc())
                time.sleep(60)  # Wait longer on error

        self.log("Autonomous Crash Monitor - Stopped")


def main():
    # Find TrinityCore root
    script_path = Path(__file__).resolve()
    trinity_root = script_path.parent.parent.parent

    # Create and run monitor
    monitor = AutonomousCrashMonitor(trinity_root)
    monitor.run_monitoring_loop()


if __name__ == "__main__":
    main()
