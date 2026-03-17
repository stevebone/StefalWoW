#!/usr/bin/env python3
"""
Enterprise Real-Time Crash Monitor with Log Analysis Integration
Monitors Server.log, Playerbot.log, and crash dumps in real-time

Features:
- Real-time log monitoring with tail -f behavior
- Crash dump correlation with log context
- Pre-crash log analysis (last 100 lines before crash)
- Error pattern detection in logs
- Warning escalation tracking
- Automated crash report with full context

Usage:
    python crash_monitor.py --server-log M:/Wplayerbot/Logs/Server.log --playerbot-log M:/Wplayerbot/Logs/Playerbot.log --crashes M:/Wplayerbot/Crashes
"""

import os
import sys
import re
import json
import time
import argparse
from pathlib import Path
from datetime import datetime, timedelta
from typing import Dict, List, Optional, Tuple
from dataclasses import dataclass, asdict
from collections import deque, defaultdict
import threading
import subprocess

# ============================================================================
# Log Entry Data Structures
# ============================================================================

@dataclass
class LogEntry:
    """Single log entry"""
    timestamp: datetime
    level: str  # DEBUG, INFO, WARN, ERROR
    category: str  # e.g., "server.worldserver", "playerbot.ai"
    message: str
    raw_line: str

@dataclass
class CrashContext:
    """Complete crash context with logs"""
    crash_file: Path
    crash_time: datetime
    server_log_pre_crash: List[LogEntry]
    playerbot_log_pre_crash: List[LogEntry]
    errors_before_crash: List[LogEntry]
    warnings_before_crash: List[LogEntry]
    last_bot_action: Optional[LogEntry]
    escalating_warnings: List[Tuple[str, int]]  # (warning_pattern, count)
    suspicious_patterns: List[str]

# ============================================================================
# Real-Time Log Monitor
# ============================================================================

class LogMonitor:
    """
    Real-time log file monitor with tail -f behavior
    """

    def __init__(self, log_file: Path, name: str, buffer_size: int = 200):
        self.log_file = Path(log_file)
        self.name = name
        self.buffer_size = buffer_size
        self.log_buffer: deque[LogEntry] = deque(maxlen=buffer_size)
        self.error_count = 0
        self.warning_count = 0
        self.running = False
        self.thread: Optional[threading.Thread] = None
        self.last_position = 0

        # Error/warning tracking
        self.error_patterns = defaultdict(int)
        self.warning_patterns = defaultdict(int)

    def start(self):
        """Start monitoring log file in background thread"""
        if self.running:
            return

        self.running = True
        self.thread = threading.Thread(target=self._monitor_loop, daemon=True)
        self.thread.start()
        print(f"📊 Started monitoring: {self.name} ({self.log_file})")

    def stop(self):
        """Stop monitoring"""
        self.running = False
        if self.thread:
            self.thread.join(timeout=2)

    def _monitor_loop(self):
        """Main monitoring loop"""
        # Seek to end of file initially
        if self.log_file.exists():
            with open(self.log_file, 'rb') as f:
                f.seek(0, 2)  # Seek to end
                self.last_position = f.tell()

        while self.running:
            try:
                if not self.log_file.exists():
                    time.sleep(1)
                    continue

                with open(self.log_file, 'r', encoding='utf-8', errors='ignore') as f:
                    f.seek(self.last_position)
                    new_lines = f.readlines()
                    self.last_position = f.tell()

                    for line in new_lines:
                        entry = self._parse_log_line(line)
                        if entry:
                            self.log_buffer.append(entry)
                            self._track_error_warning(entry)

                time.sleep(0.1)  # Check every 100ms

            except Exception as e:
                print(f"⚠️  Error monitoring {self.name}: {e}")
                time.sleep(1)

    def _parse_log_line(self, line: str) -> Optional[LogEntry]:
        """Parse single log line into LogEntry"""
        line = line.strip()
        if not line:
            return None

        # TrinityCore log format:
        # 2025-10-30 01:05:34 [DEBUG] [server.worldserver]: Message
        # 2025-10-30 01:05:34 ERROR: Message (older format)

        # Try new format first
        match = re.match(
            r'(\d{4}-\d{2}-\d{2}\s+\d{2}:\d{2}:\d{2})\s+\[(\w+)\]\s+\[([^\]]+)\]:\s*(.*)',
            line
        )
        if match:
            timestamp_str, level, category, message = match.groups()
            timestamp = datetime.strptime(timestamp_str, '%Y-%m-%d %H:%M:%S')
            return LogEntry(
                timestamp=timestamp,
                level=level,
                category=category,
                message=message,
                raw_line=line
            )

        # Try old format
        match = re.match(
            r'(\d{4}-\d{2}-\d{2}\s+\d{2}:\d{2}:\d{2})\s+(\w+):\s*(.*)',
            line
        )
        if match:
            timestamp_str, level, message = match.groups()
            timestamp = datetime.strptime(timestamp_str, '%Y-%m-%d %H:%M:%S')
            return LogEntry(
                timestamp=timestamp,
                level=level,
                category="unknown",
                message=message,
                raw_line=line
            )

        # Couldn't parse - return as INFO
        return LogEntry(
            timestamp=datetime.now(),
            level="INFO",
            category="unknown",
            message=line,
            raw_line=line
        )

    def _track_error_warning(self, entry: LogEntry):
        """Track errors and warnings for pattern detection"""
        if entry.level == "ERROR":
            self.error_count += 1
            # Extract pattern (first 50 chars)
            pattern = entry.message[:50]
            self.error_patterns[pattern] += 1

        elif entry.level in ["WARN", "WARNING"]:
            self.warning_count += 1
            pattern = entry.message[:50]
            self.warning_patterns[pattern] += 1

    def get_recent_entries(self, seconds: int = 60) -> List[LogEntry]:
        """Get log entries from last N seconds"""
        cutoff = datetime.now() - timedelta(seconds=seconds)
        return [
            entry for entry in self.log_buffer
            if entry.timestamp >= cutoff
        ]

    def get_errors_before_time(self, timestamp: datetime, seconds: int = 60) -> List[LogEntry]:
        """Get all errors before a specific timestamp"""
        cutoff = timestamp - timedelta(seconds=seconds)
        return [
            entry for entry in self.log_buffer
            if entry.level == "ERROR"
            and cutoff <= entry.timestamp <= timestamp
        ]

    def get_warnings_before_time(self, timestamp: datetime, seconds: int = 60) -> List[LogEntry]:
        """Get all warnings before a specific timestamp"""
        cutoff = timestamp - timedelta(seconds=seconds)
        return [
            entry for entry in self.log_buffer
            if entry.level in ["WARN", "WARNING"]
            and cutoff <= entry.timestamp <= timestamp
        ]

    def get_escalating_warnings(self, threshold: int = 5) -> List[Tuple[str, int]]:
        """Get warnings that are repeating frequently (escalating)"""
        escalating = []
        for pattern, count in self.warning_patterns.items():
            if count >= threshold:
                escalating.append((pattern, count))
        return sorted(escalating, key=lambda x: -x[1])

    def find_last_bot_action(self, timestamp: datetime) -> Optional[LogEntry]:
        """Find the last bot-related action before crash"""
        cutoff = timestamp - timedelta(seconds=10)
        bot_keywords = ['bot', 'botsession', 'botai', 'playerbot', 'deathrecovery']

        for entry in reversed(list(self.log_buffer)):
            if entry.timestamp > timestamp:
                continue
            if entry.timestamp < cutoff:
                break

            if any(keyword in entry.message.lower() for keyword in bot_keywords):
                return entry

        return None

# ============================================================================
# Crash Context Analyzer
# ============================================================================

class CrashContextAnalyzer:
    """
    Analyzes crash dumps with full log context
    """

    def __init__(
        self,
        server_monitor: LogMonitor,
        playerbot_monitor: LogMonitor,
        crashes_dir: Path
    ):
        self.server_monitor = server_monitor
        self.playerbot_monitor = playerbot_monitor
        self.crashes_dir = Path(crashes_dir)
        self.processed_crashes: set[Path] = set()

    def analyze_crash_with_context(self, crash_file: Path) -> CrashContext:
        """Analyze crash dump with full log context"""

        # Get crash time from filename or file modification time
        crash_time = self._extract_crash_time(crash_file)

        # Get pre-crash log entries (60 seconds before crash)
        server_pre_crash = self.server_monitor.get_recent_entries(seconds=60)
        playerbot_pre_crash = self.playerbot_monitor.get_recent_entries(seconds=60)

        # Get errors and warnings before crash
        errors = self.server_monitor.get_errors_before_time(crash_time, seconds=60)
        errors.extend(self.playerbot_monitor.get_errors_before_time(crash_time, seconds=60))

        warnings = self.server_monitor.get_warnings_before_time(crash_time, seconds=60)
        warnings.extend(self.playerbot_monitor.get_warnings_before_time(crash_time, seconds=60))

        # Get escalating warnings
        escalating = self.server_monitor.get_escalating_warnings(threshold=5)
        escalating.extend(self.playerbot_monitor.get_escalating_warnings(threshold=5))

        # Find last bot action
        last_bot_action = self.playerbot_monitor.find_last_bot_action(crash_time)

        # Detect suspicious patterns
        suspicious = self._detect_suspicious_patterns(server_pre_crash + playerbot_pre_crash)

        return CrashContext(
            crash_file=crash_file,
            crash_time=crash_time,
            server_log_pre_crash=server_pre_crash[-50:],  # Last 50 entries
            playerbot_log_pre_crash=playerbot_pre_crash[-50:],
            errors_before_crash=errors,
            warnings_before_crash=warnings[-20:],  # Last 20 warnings
            last_bot_action=last_bot_action,
            escalating_warnings=escalating[:10],  # Top 10
            suspicious_patterns=suspicious
        )

    def _extract_crash_time(self, crash_file: Path) -> datetime:
        """Extract crash timestamp from filename or file mtime"""
        # Try to parse from filename: worldserver.exe_[2025_10_30_1_5_34].txt
        match = re.search(r'(\d{4})_(\d{1,2})_(\d{1,2})_(\d{1,2})_(\d{1,2})_(\d{1,2})', crash_file.name)
        if match:
            year, month, day, hour, minute, second = map(int, match.groups())
            return datetime(year, month, day, hour, minute, second)

        # Fall back to file modification time
        return datetime.fromtimestamp(crash_file.stat().st_mtime)

    def _detect_suspicious_patterns(self, log_entries: List[LogEntry]) -> List[str]:
        """Detect suspicious patterns in pre-crash logs"""
        suspicious = []

        # Pattern 1: Rapid repeated errors
        error_messages = [e.message for e in log_entries if e.level == "ERROR"]
        if len(error_messages) > 10:
            # Check for repeated errors
            from collections import Counter
            counts = Counter(error_messages)
            for msg, count in counts.most_common(3):
                if count >= 5:
                    suspicious.append(f"Repeated error ({count}x): {msg[:60]}")

        # Pattern 2: Memory warnings
        memory_keywords = ['memory', 'allocation', 'heap', 'leak', 'out of memory']
        memory_warnings = [
            e for e in log_entries
            if any(kw in e.message.lower() for kw in memory_keywords)
        ]
        if memory_warnings:
            suspicious.append(f"Memory-related warnings detected ({len(memory_warnings)})")

        # Pattern 3: Null pointer warnings
        null_keywords = ['null', 'nullptr', 'invalid pointer']
        null_warnings = [
            e for e in log_entries
            if any(kw in e.message.lower() for kw in null_keywords)
        ]
        if null_warnings:
            suspicious.append(f"Null pointer warnings detected ({len(null_warnings)})")

        # Pattern 4: Deadlock indicators
        deadlock_keywords = ['deadlock', 'timeout', 'waiting for lock', 'mutex']
        deadlock_warnings = [
            e for e in log_entries
            if any(kw in e.message.lower() for kw in deadlock_keywords)
        ]
        if deadlock_warnings:
            suspicious.append(f"Potential deadlock indicators ({len(deadlock_warnings)})")

        # Pattern 5: Database errors
        db_keywords = ['database', 'sql', 'query failed', 'connection lost']
        db_errors = [
            e for e in log_entries
            if e.level == "ERROR" and any(kw in e.message.lower() for kw in db_keywords)
        ]
        if db_errors:
            suspicious.append(f"Database errors detected ({len(db_errors)})")

        return suspicious

    def print_crash_context_report(self, context: CrashContext, crash_info: dict):
        """Print comprehensive crash report with full context"""
        print("\n" + "="*100)
        print(f"🔥 COMPREHENSIVE CRASH ANALYSIS WITH LOG CONTEXT")
        print("="*100)

        print(f"\n📅 Crash Timestamp: {context.crash_time.strftime('%Y-%m-%d %H:%M:%S')}")
        print(f"📁 Crash File: {context.crash_file.name}")

        # Print crash dump info (from crash_analyzer.py)
        if crash_info:
            print(f"\n💥 Crash Details:")
            print(f"   Location: {crash_info.get('crash_location', 'Unknown')}")
            print(f"   Function: {crash_info.get('crash_function', 'Unknown')}")
            print(f"   Error: {crash_info.get('error_message', 'Unknown')}")
            print(f"   Severity: {crash_info.get('severity', 'Unknown')}")

        # Errors before crash
        if context.errors_before_crash:
            print(f"\n❌ ERRORS BEFORE CRASH ({len(context.errors_before_crash)}):")
            for i, error in enumerate(context.errors_before_crash[-10:], 1):
                print(f"   {i:2d}. [{error.timestamp.strftime('%H:%M:%S')}] {error.message[:80]}")

        # Escalating warnings
        if context.escalating_warnings:
            print(f"\n⚠️  ESCALATING WARNINGS (Repeated 5+ times):")
            for pattern, count in context.escalating_warnings[:5]:
                print(f"   - ({count:3d}x) {pattern}")

        # Last bot action
        if context.last_bot_action:
            print(f"\n🤖 LAST BOT ACTION:")
            print(f"   [{context.last_bot_action.timestamp.strftime('%H:%M:%S')}] {context.last_bot_action.message}")

        # Suspicious patterns
        if context.suspicious_patterns:
            print(f"\n🚨 SUSPICIOUS PATTERNS DETECTED:")
            for pattern in context.suspicious_patterns:
                print(f"   - {pattern}")

        # Server log context
        print(f"\n📋 SERVER.LOG CONTEXT (Last 20 entries):")
        for i, entry in enumerate(context.server_log_pre_crash[-20:], 1):
            level_icon = {
                "ERROR": "❌",
                "WARN": "⚠️ ",
                "WARNING": "⚠️ ",
                "INFO": "ℹ️ ",
                "DEBUG": "🔍"
            }.get(entry.level, "  ")
            print(f"   {i:2d}. {level_icon} [{entry.timestamp.strftime('%H:%M:%S')}] {entry.message[:90]}")

        # Playerbot log context
        print(f"\n🤖 PLAYERBOT.LOG CONTEXT (Last 20 entries):")
        for i, entry in enumerate(context.playerbot_log_pre_crash[-20:], 1):
            level_icon = {
                "ERROR": "❌",
                "WARN": "⚠️ ",
                "WARNING": "⚠️ ",
                "INFO": "ℹ️ ",
                "DEBUG": "🔍"
            }.get(entry.level, "  ")
            print(f"   {i:2d}. {level_icon} [{entry.timestamp.strftime('%H:%M:%S')}] {entry.message[:90]}")

        print("\n" + "="*100 + "\n")

    def save_context_report(self, context: CrashContext, crash_info: dict, output_dir: Path):
        """Save crash context report to JSON and Markdown"""
        output_dir = Path(output_dir)
        output_dir.mkdir(parents=True, exist_ok=True)

        crash_id = crash_info.get('crash_id', 'unknown') if crash_info else 'unknown'
        timestamp_str = context.crash_time.strftime('%Y%m%d_%H%M%S')

        # Save as JSON
        json_file = output_dir / f"crash_context_{crash_id}_{timestamp_str}.json"
        json_data = {
            "crash_file": str(context.crash_file),
            "crash_time": context.crash_time.isoformat(),
            "crash_info": crash_info,
            "errors_before_crash": [asdict(e) for e in context.errors_before_crash],
            "warnings_before_crash": [asdict(e) for e in context.warnings_before_crash],
            "server_log_context": [asdict(e) for e in context.server_log_pre_crash],
            "playerbot_log_context": [asdict(e) for e in context.playerbot_log_pre_crash],
            "last_bot_action": asdict(context.last_bot_action) if context.last_bot_action else None,
            "escalating_warnings": context.escalating_warnings,
            "suspicious_patterns": context.suspicious_patterns
        }

        with open(json_file, 'w', encoding='utf-8') as f:
            json.dump(json_data, f, indent=2, default=str)

        print(f"💾 Crash context saved: {json_file}")

        # Save as Markdown
        md_file = output_dir / f"crash_report_{crash_id}_{timestamp_str}.md"
        self._save_markdown_report(context, crash_info, md_file)
        print(f"📝 Markdown report saved: {md_file}")

    def _save_markdown_report(self, context: CrashContext, crash_info: dict, md_file: Path):
        """Save formatted Markdown crash report"""
        with open(md_file, 'w', encoding='utf-8') as f:
            f.write(f"# Crash Report - {context.crash_time.strftime('%Y-%m-%d %H:%M:%S')}\n\n")

            if crash_info:
                f.write(f"## Crash Details\n\n")
                f.write(f"- **Crash ID**: `{crash_info.get('crash_id', 'unknown')}`\n")
                f.write(f"- **Location**: `{crash_info.get('crash_location', 'Unknown')}`\n")
                f.write(f"- **Function**: `{crash_info.get('crash_function', 'Unknown')}`\n")
                f.write(f"- **Error**: {crash_info.get('error_message', 'Unknown')}\n")
                f.write(f"- **Severity**: {crash_info.get('severity', 'Unknown')}\n")
                f.write(f"- **Root Cause**: {crash_info.get('root_cause_hypothesis', 'Unknown')}\n\n")

            if context.errors_before_crash:
                f.write(f"## Errors Before Crash ({len(context.errors_before_crash)})\n\n")
                for error in context.errors_before_crash[-15:]:
                    f.write(f"- `[{error.timestamp.strftime('%H:%M:%S')}]` {error.message}\n")
                f.write("\n")

            if context.escalating_warnings:
                f.write(f"## Escalating Warnings (Repeated 5+ times)\n\n")
                for pattern, count in context.escalating_warnings[:10]:
                    f.write(f"- `({count}x)` {pattern}\n")
                f.write("\n")

            if context.last_bot_action:
                f.write(f"## Last Bot Action\n\n")
                f.write(f"`[{context.last_bot_action.timestamp.strftime('%H:%M:%S')}]` {context.last_bot_action.message}\n\n")

            if context.suspicious_patterns:
                f.write(f"## Suspicious Patterns\n\n")
                for pattern in context.suspicious_patterns:
                    f.write(f"- {pattern}\n")
                f.write("\n")

            f.write(f"## Server.log Context (Last 30 entries)\n\n")
            f.write("```\n")
            for entry in context.server_log_pre_crash[-30:]:
                f.write(f"[{entry.timestamp.strftime('%H:%M:%S')}] [{entry.level}] {entry.message}\n")
            f.write("```\n\n")

            f.write(f"## Playerbot.log Context (Last 30 entries)\n\n")
            f.write("```\n")
            for entry in context.playerbot_log_pre_crash[-30:]:
                f.write(f"[{entry.timestamp.strftime('%H:%M:%S')}] [{entry.level}] {entry.message}\n")
            f.write("```\n\n")

            if crash_info and crash_info.get('call_stack'):
                f.write(f"## Call Stack\n\n")
                f.write("```\n")
                for frame in crash_info['call_stack']:
                    f.write(f"{frame}\n")
                f.write("```\n\n")

# ============================================================================
# Real-Time Crash Monitor
# ============================================================================

class RealTimeCrashMonitor:
    """
    Complete real-time crash monitoring system
    """

    def __init__(
        self,
        server_log: Path,
        playerbot_log: Path,
        crashes_dir: Path,
        trinity_root: Path
    ):
        self.server_monitor = LogMonitor(server_log, "Server.log")
        self.playerbot_monitor = LogMonitor(playerbot_log, "Playerbot.log")
        self.context_analyzer = CrashContextAnalyzer(
            self.server_monitor,
            self.playerbot_monitor,
            crashes_dir
        )
        self.crashes_dir = Path(crashes_dir)
        self.trinity_root = Path(trinity_root)
        self.processed_crashes: set[Path] = set()
        self.running = False

    def start(self):
        """Start real-time monitoring"""
        print("\n" + "="*100)
        print("🚀 STARTING ENTERPRISE CRASH MONITOR")
        print("="*100)
        print(f"\n📊 Monitoring Configuration:")
        print(f"   Server Log: {self.server_monitor.log_file}")
        print(f"   Playerbot Log: {self.playerbot_monitor.log_file}")
        print(f"   Crashes Dir: {self.crashes_dir}")
        print(f"   Trinity Root: {self.trinity_root}")
        print("\n   Press Ctrl+C to stop\n")

        # Start log monitors
        self.server_monitor.start()
        self.playerbot_monitor.start()

        # Wait for monitors to initialize
        time.sleep(2)

        # Start crash detection loop
        self.running = True
        try:
            self._monitor_crashes()
        except KeyboardInterrupt:
            print("\n\n🛑 Stopping crash monitor...")
            self.stop()

    def stop(self):
        """Stop all monitoring"""
        self.running = False
        self.server_monitor.stop()
        self.playerbot_monitor.stop()
        print("✅ Crash monitor stopped")

    def _monitor_crashes(self):
        """Monitor for new crash dumps"""
        print("👁️  Watching for crash dumps...\n")

        # Get initial set of crashes
        if self.crashes_dir.exists():
            self.processed_crashes = set(self.crashes_dir.glob("*worldserver*.txt"))

        iteration = 0
        while self.running:
            iteration += 1

            # Print status every 10 iterations (10 seconds)
            if iteration % 10 == 0:
                print(f"[{datetime.now().strftime('%H:%M:%S')}] Monitoring... "
                      f"(Errors: S={self.server_monitor.error_count} "
                      f"P={self.playerbot_monitor.error_count}, "
                      f"Warnings: S={self.server_monitor.warning_count} "
                      f"P={self.playerbot_monitor.warning_count})")

            # Check for new crashes
            if self.crashes_dir.exists():
                current_crashes = set(self.crashes_dir.glob("*worldserver*.txt"))
                new_crashes = current_crashes - self.processed_crashes

                for crash_file in new_crashes:
                    self._handle_new_crash(crash_file)
                    self.processed_crashes.add(crash_file)

            time.sleep(1)

    def _handle_new_crash(self, crash_file: Path):
        """Handle newly detected crash"""
        print("\n" + "🔥" * 50)
        print(f"💥 NEW CRASH DETECTED: {crash_file.name}")
        print("🔥" * 50 + "\n")

        # Wait a moment for crash dump to be fully written
        time.sleep(2)

        # Analyze crash dump (using crash_analyzer.py if available)
        crash_info = self._parse_crash_dump_basic(crash_file)

        # Analyze with full context
        context = self.context_analyzer.analyze_crash_with_context(crash_file)

        # Print comprehensive report
        self.context_analyzer.print_crash_context_report(context, crash_info)

        # Save reports
        reports_dir = self.trinity_root / ".claude" / "crash_reports"
        self.context_analyzer.save_context_report(context, crash_info, reports_dir)

        print(f"✅ Crash analysis complete\n")

    def _parse_crash_dump_basic(self, crash_file: Path) -> dict:
        """Basic crash dump parsing (simplified version)"""
        try:
            with open(crash_file, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()

            exception_match = re.search(r'Exception: ([A-F0-9]+)', content)
            location_match = re.search(r'([A-Za-z0-9_]+\.(cpp|h)):(\d+)', content)
            func_match = re.search(r'([A-Za-z0-9_:]+::[A-Za-z0-9_]+)\s*\(', content)
            error_match = re.search(r'(?:Error|throw|Exception):\s*"?([^"\n]+)"?', content, re.IGNORECASE)

            # Extract call stack
            stack_section = re.search(r'Call stack:(.*?)(?:\n\n|\Z)', content, re.DOTALL | re.IGNORECASE)
            call_stack = []
            if stack_section:
                stack_lines = stack_section.group(1).strip().split('\n')
                for line in stack_lines[:15]:
                    frame = re.sub(r'^\s*\d+\s+', '', line).strip()
                    if frame:
                        call_stack.append(frame)

            return {
                "crash_id": hashlib.md5(str(crash_file).encode()).hexdigest()[:12],
                "exception_code": exception_match.group(1) if exception_match else "UNKNOWN",
                "crash_location": f"{location_match.group(1)}:{location_match.group(3)}" if location_match else "Unknown",
                "crash_function": func_match.group(1) if func_match else "Unknown",
                "error_message": error_match.group(1).strip() if error_match else "No error message",
                "call_stack": call_stack,
                "severity": "CRITICAL",
                "root_cause_hypothesis": "See detailed analysis"
            }

        except Exception as e:
            print(f"⚠️  Error parsing crash dump: {e}")
            return {"crash_id": "error", "severity": "UNKNOWN"}

# ============================================================================
# Main Entry Point
# ============================================================================

def main():
    parser = argparse.ArgumentParser(
        description="Enterprise Real-Time Crash Monitor with Log Analysis"
    )

    parser.add_argument(
        '--server-log',
        type=Path,
        default=Path('M:/Wplayerbot/Logs/Server.log'),
        help='Server.log file path'
    )

    parser.add_argument(
        '--playerbot-log',
        type=Path,
        default=Path('M:/Wplayerbot/Logs/Playerbot.log'),
        help='Playerbot.log file path'
    )

    parser.add_argument(
        '--crashes',
        type=Path,
        default=Path('M:/Wplayerbot/Crashes'),
        help='Crash dumps directory'
    )

    parser.add_argument(
        '--trinity-root',
        type=Path,
        default=Path('c:/TrinityBots/TrinityCore'),
        help='TrinityCore root directory'
    )

    args = parser.parse_args()

    # Create monitor
    monitor = RealTimeCrashMonitor(
        server_log=args.server_log,
        playerbot_log=args.playerbot_log,
        crashes_dir=args.crashes,
        trinity_root=args.trinity_root
    )

    # Start monitoring
    monitor.start()

if __name__ == "__main__":
    import hashlib  # Add missing import
    main()
