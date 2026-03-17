#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Test the enhanced crash parser with actual crash dump
"""

import sys
import io
from pathlib import Path

# Set UTF-8 encoding for console output
if sys.platform == 'win32':
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
    sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding='utf-8', errors='replace')

# Add parent directory to path
sys.path.insert(0, str(Path(__file__).parent))

from crash_analyzer import CrashAnalyzer

def main():
    print("=" * 80)
    print("Testing Enhanced Crash Parser")
    print("=" * 80)

    trinity_root = Path("c:/TrinityBots/TrinityCore")
    crashes_dir = Path("M:/Wplayerbot/Crashes")

    # Find most recent crash
    crash_files = sorted(crashes_dir.glob("*.txt"), key=lambda p: p.stat().st_mtime, reverse=True)

    if not crash_files:
        print("❌ No crash files found")
        return 1

    crash_file = crash_files[0]
    print(f"\n📄 Testing with: {crash_file.name}")
    print(f"   Size: {crash_file.stat().st_size:,} bytes")

    # Create analyzer
    analyzer = CrashAnalyzer(trinity_root, crashes_dir)

    # Parse crash
    print("\n🔍 Parsing crash dump...")
    crash_info = analyzer.parse_crash_dump(crash_file)

    if not crash_info:
        print("❌ Failed to parse crash dump")
        return 1

    # Print results
    print("\n" + "=" * 80)
    print("✅ CRASH ANALYSIS RESULTS")
    print("=" * 80)

    print(f"\n📊 Basic Information:")
    print(f"   Crash ID: {crash_info.crash_id}")
    print(f"   Exception Code: {crash_info.exception_code}")
    print(f"   Fault Address: {crash_info.exception_address}")
    print(f"   Severity: {crash_info.severity}")
    print(f"   Category: {crash_info.crash_category}")

    print(f"\n🎯 Crash Location:")
    print(f"   File:Line: {crash_info.crash_location}")
    print(f"   Function: {crash_info.crash_function}")
    print(f"   Error: {crash_info.error_message}")

    print(f"\n📚 Call Stack (Top 10):")
    for i, frame in enumerate(crash_info.call_stack[:10], 1):
        print(f"   {i:2}. {frame}")

    print(f"\n🤖 Bot Related: {crash_info.is_bot_related}")
    print(f"   Affected Components: {', '.join(crash_info.affected_components)}")

    print(f"\n💡 Root Cause Hypothesis:")
    print(f"   {crash_info.root_cause_hypothesis}")

    print(f"\n🔧 Fix Suggestions:")
    for i, suggestion in enumerate(crash_info.fix_suggestions, 1):
        print(f"   {i}. {suggestion}")

    print("\n" + "=" * 80)
    print("✅ TEST COMPLETE")
    print("=" * 80)

    # Validation
    print("\n🎯 Validation:")
    if crash_info.exception_code != "UNKNOWN":
        print("   ✅ Exception code parsed correctly")
    else:
        print("   ❌ Exception code still UNKNOWN")

    if crash_info.exception_address != "UNKNOWN":
        print("   ✅ Fault address parsed correctly")
    else:
        print("   ❌ Fault address still UNKNOWN")

    if crash_info.crash_location != "Unknown":
        print("   ✅ Crash location identified")
    else:
        print("   ❌ Crash location still Unknown")

    if crash_info.crash_function != "Unknown":
        print("   ✅ Crash function identified")
    else:
        print("   ❌ Crash function still Unknown")

    if len(crash_info.call_stack) > 0:
        print(f"   ✅ Call stack captured ({len(crash_info.call_stack)} frames)")
    else:
        print("   ❌ Call stack empty")

    return 0

if __name__ == "__main__":
    sys.exit(main())
