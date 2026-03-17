#!/usr/bin/env python3
"""
Claude Code Auto-Processor
===========================

This script runs INSIDE Claude Code's execution context and implements
the periodic checking and auto-processing of crash analysis requests.

When Claude Code starts up or when explicitly invoked, this script:
1. Checks for auto-process marker files every 30 seconds
2. Reads the crash analysis request
3. Performs comprehensive analysis using Trinity MCP + Serena MCP
4. Generates enterprise-grade fix
5. Writes response JSON
6. Applies fix to filesystem

This is the "Claude Code side" of the autonomous system.
"""

import json
import time
import sys
from pathlib import Path
from datetime import datetime
from typing import Dict, List, Optional


class ClaudeAutoProcessor:
    """
    Autonomous processor that runs within Claude Code context

    Detects marker files and triggers crash analysis automatically
    """

    def __init__(self, trinity_root: Path):
        self.trinity_root = trinity_root
        self.queue_dir = trinity_root / ".claude/crash_analysis_queue"
        self.requests_dir = self.queue_dir / "requests"
        self.responses_dir = self.queue_dir / "responses"
        self.markers_dir = self.queue_dir / "auto_process"
        self.log_file = trinity_root / ".claude/logs/claude_auto_processor.log"

        # Create directories
        self.log_file.parent.mkdir(parents=True, exist_ok=True)
        self.markers_dir.mkdir(parents=True, exist_ok=True)

        self.processed = set()

    def log(self, message: str, level: str = "INFO"):
        """Log with timestamp"""
        timestamp = datetime.now().isoformat()
        log_msg = f"[{timestamp}] [Claude] [{level}] {message}"

        print(log_msg)

        try:
            with open(self.log_file, 'a', encoding='utf-8') as f:
                f.write(log_msg + '\n')
        except:
            pass

    def find_marker_files(self) -> List[Dict]:
        """
        Find all auto-process marker files

        Returns list of marker info dicts
        """
        markers = []

        if not self.markers_dir.exists():
            return markers

        for marker_file in self.markers_dir.glob("process_*.json"):
            try:
                with open(marker_file, 'r', encoding='utf-8') as f:
                    marker_data = json.load(f)

                request_id = marker_data['request_id']

                # Skip if already processed
                if request_id in self.processed:
                    continue

                # Check if response already exists
                response_file = self.responses_dir / f"response_{request_id}.json"
                if response_file.exists():
                    # Response exists, clean up marker
                    marker_file.unlink()
                    self.processed.add(request_id)
                    continue

                markers.append({
                    'marker_file': marker_file,
                    'request_id': request_id,
                    'crash_id': marker_data.get('crash_id', 'unknown'),
                    'request_file': Path(marker_data['request_file']),
                    'data': marker_data
                })

            except Exception as e:
                self.log(f"ERROR reading marker {marker_file}: {e}", "ERROR")
                continue

        return markers

    def read_crash_analysis_file(self, crash_id: str) -> Optional[str]:
        """
        Find and read the CDB analysis file for a crash

        Returns the analysis content or None if not found
        """
        crash_dirs = [
            Path("M:/Wplayerbot/Crashes"),
            self.trinity_root / "crashes"
        ]

        for crash_dir in crash_dirs:
            if not crash_dir.exists():
                continue

            # Look for CDB analysis file
            for file in crash_dir.glob(f"*{crash_id}*.cdb_analysis.txt"):
                try:
                    self.log(f"Found CDB analysis: {file}")
                    with open(file, 'r', encoding='utf-8', errors='ignore') as f:
                        return f.read()
                except Exception as e:
                    self.log(f"ERROR reading {file}: {e}", "ERROR")

        return None

    def trigger_claude_analysis(self, marker_info: Dict) -> bool:
        """
        CRITICAL: This is where Claude Code's analysis logic is invoked

        In a real implementation, this would:
        1. Call Claude Code's /analyze-crash command programmatically
        2. Or directly invoke the analysis logic
        3. Or create a signal that Claude Code's main loop detects

        For now, this creates a detailed instruction file that tells
        Claude Code exactly what to do.
        """
        request_id = marker_info['request_id']
        crash_id = marker_info['crash_id']

        self.log("")
        self.log("╔══════════════════════════════════════════════════════════")
        self.log("║ AUTONOMOUS ANALYSIS TRIGGER")
        self.log("╠══════════════════════════════════════════════════════════")
        self.log(f"║ Request ID: {request_id}")
        self.log(f"║ Crash ID: {crash_id}")
        self.log("╚══════════════════════════════════════════════════════════")
        self.log("")

        try:
            # Read the request
            with open(marker_info['request_file'], 'r', encoding='utf-8') as f:
                request_data = json.load(f)

            # Read crash analysis
            cdb_analysis = self.read_crash_analysis_file(crash_id)

            if not cdb_analysis:
                self.log(f"ERROR: No CDB analysis found for {crash_id}", "ERROR")
                return False

            # Create analysis instruction file
            instruction_file = self.markers_dir / f"ANALYZE_{request_id}.txt"

            instruction = f"""
=============================================================================
AUTONOMOUS CRASH ANALYSIS INSTRUCTION
=============================================================================

Request ID: {request_id}
Crash ID: {crash_id}
Timestamp: {datetime.now().isoformat()}

ACTION REQUIRED:
You (Claude Code) must now perform comprehensive crash analysis for this
crash and generate an enterprise-grade fix.

STEPS:
1. Read the CDB analysis below
2. Use Trinity MCP to research relevant APIs and patterns
3. Use Serena MCP to analyze the codebase and find implementation details
4. Identify root cause with precision
5. Design comprehensive fix (no shortcuts!)
6. Write response JSON to: {self.responses_dir}/response_{request_id}.json
7. Apply fix to the appropriate source file(s)
8. Delete this instruction file when complete

REQUEST DATA:
{json.dumps(request_data, indent=2)}

CDB ANALYSIS:
{cdb_analysis}

=============================================================================
BEGIN ANALYSIS NOW
=============================================================================
"""

            with open(instruction_file, 'w', encoding='utf-8') as f:
                f.write(instruction)

            self.log(f"✅ Created analysis instruction: {instruction_file}")
            self.log(f"   Claude Code should now process this automatically")

            # Mark as triggered
            self.processed.add(request_id)

            return True

        except Exception as e:
            self.log(f"ERROR triggering analysis: {e}", "ERROR")
            import traceback
            self.log(traceback.format_exc(), "DEBUG")
            return False

    def process_markers(self):
        """
        Process all detected marker files

        This is called periodically by the monitoring loop
        """
        markers = self.find_marker_files()

        if not markers:
            return

        self.log(f"🔔 DETECTED {len(markers)} MARKER FILE(S)")

        for marker_info in markers:
            self.log(f"Processing marker for request {marker_info['request_id']}")

            success = self.trigger_claude_analysis(marker_info)

            if success:
                # Clean up marker file
                try:
                    marker_info['marker_file'].unlink()
                    self.log(f"✅ Cleaned up marker: {marker_info['marker_file'].name}")
                except:
                    pass
            else:
                self.log(f"❌ Failed to trigger analysis for {marker_info['request_id']}", "ERROR")

    def run_monitoring_loop(self):
        """
        Main loop - checks for markers every 30 seconds

        This is the "Claude Code side" autonomous processor
        """
        self.log("═" * 70)
        self.log("CLAUDE CODE AUTO-PROCESSOR - STARTED")
        self.log("═" * 70)
        self.log(f"Trinity Root: {self.trinity_root}")
        self.log(f"Markers Dir: {self.markers_dir}")
        self.log(f"Check Interval: 30 seconds")
        self.log("═" * 70)
        self.log("")
        self.log("🤖 Monitoring for auto-process markers...")
        self.log("")

        iteration = 0

        while True:
            try:
                iteration += 1

                # Heartbeat every 5 minutes
                if iteration % 10 == 0:
                    self.log(f"💓 Heartbeat: Iteration {iteration}, Processed: {len(self.processed)}")

                # Check for markers
                self.process_markers()

                # Sleep
                time.sleep(30)

            except KeyboardInterrupt:
                self.log("")
                self.log("🛑 Shutdown requested")
                break

            except Exception as e:
                self.log(f"ERROR in loop: {e}", "ERROR")
                import traceback
                self.log(traceback.format_exc(), "DEBUG")
                time.sleep(60)

        self.log("Claude Code Auto-Processor - STOPPED")


def main():
    """Entry point"""
    if hasattr(sys.stdout, 'reconfigure'):
        sys.stdout.reconfigure(encoding='utf-8')

    script_path = Path(__file__).resolve()
    trinity_root = script_path.parent.parent.parent

    print(f"TrinityCore Root: {trinity_root}")
    print("")

    processor = ClaudeAutoProcessor(trinity_root)
    processor.run_monitoring_loop()


if __name__ == "__main__":
    main()
