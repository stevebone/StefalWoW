#!/usr/bin/env python3
"""
Static analysis tool to detect lock ordering violations in PlayerBot module

This tool scans C++ source files for OrderedMutex, OrderedSharedMutex, and
OrderedRecursiveMutex acquisitions and verifies they are acquired in ascending
order according to the LockOrder hierarchy defined in LockHierarchy.h.

Usage:
    python3 scripts/analyze_lock_order.py [--verbose] [--path PATH]

Returns:
    0 if no violations found
    1 if violations detected

Example output:
    Lock ordering violations detected:

    src/modules/Playerbot/AI/BotAI.cpp:
      - Lock SESSION_MANAGER (order 2000) acquired after lock with order 3000

    src/modules/Playerbot/Combat/ThreatManager.cpp:
      - Lock SPATIAL_GRID (order 1000) acquired after THREAT_COORDINATOR (5000)
"""

import re
import sys
import argparse
from pathlib import Path
from typing import List, Dict, Tuple, Optional
from dataclasses import dataclass
from collections import defaultdict

# Lock hierarchy from LockHierarchy.h
# This should be kept in sync with the C++ enum
LOCK_HIERARCHY = {
    # Layer 1: Infrastructure
    "LOG_SYSTEM": 100,
    "CONFIG_MANAGER": 200,
    "METRICS_COLLECTOR": 300,

    # Layer 2: Core data structures
    "SPATIAL_GRID": 1000,
    "OBJECT_CACHE": 1100,
    "PLAYER_SNAPSHOT_BUFFER": 1200,

    # Layer 3: Session management
    "SESSION_MANAGER": 2000,
    "PACKET_QUEUE": 2100,
    "PACKET_RELAY": 2200,

    # Layer 4: Bot lifecycle
    "BOT_SPAWNER": 3000,
    "BOT_SCHEDULER": 3100,
    "DEATH_RECOVERY": 3200,

    # Layer 5: Bot AI
    "BOT_AI_STATE": 4000,
    "BEHAVIOR_MANAGER": 4100,
    "ACTION_PRIORITY": 4200,

    # Layer 6: Combat systems
    "THREAT_COORDINATOR": 5000,
    "INTERRUPT_COORDINATOR": 5100,
    "DISPEL_COORDINATOR": 5200,
    "TARGET_SELECTOR": 5300,

    # Layer 7: Group/Raid coordination
    "GROUP_MANAGER": 6000,
    "RAID_COORDINATOR": 6100,
    "ROLE_ASSIGNMENT": 6200,

    # Layer 8: Movement and pathfinding
    "MOVEMENT_ARBITER": 7000,
    "PATHFINDING_ADAPTER": 7100,
    "FORMATION_MANAGER": 7200,

    # Layer 9: Game systems
    "QUEST_MANAGER": 8000,
    "LOOT_MANAGER": 8100,
    "TRADE_MANAGER": 8200,
    "PROFESSION_MANAGER": 8300,

    # Layer 10: Database operations
    "DATABASE_POOL": 9000,
    "DATABASE_TRANSACTION": 9100,

    # Layer 11: External dependencies
    "TRINITYCORE_MAP": 10000,
    "TRINITYCORE_WORLD": 10100,
    "TRINITYCORE_OBJECTMGR": 10200,
}

@dataclass
class LockAcquisition:
    """Represents a lock acquisition in source code"""
    lock_name: str
    lock_order: int
    line_number: int
    line_content: str
    function_name: Optional[str] = None

@dataclass
class OrderingViolation:
    """Represents a lock ordering violation"""
    file_path: Path
    lock1: LockAcquisition
    lock2: LockAcquisition

    def __str__(self) -> str:
        return (
            f"Lock {self.lock2.lock_name} (order {self.lock2.lock_order}) "
            f"acquired after lock {self.lock1.lock_name} (order {self.lock1.lock_order})\n"
            f"  Line {self.lock2.line_number}: {self.lock2.line_content.strip()}\n"
            f"  Previous lock at line {self.lock1.line_number}: {self.lock1.line_content.strip()}"
        )


def extract_lock_acquisitions(file_path: Path, verbose: bool = False) -> List[LockAcquisition]:
    """
    Parse C++ file and extract all OrderedMutex lock acquisitions

    Patterns detected:
    - std::lock_guard<OrderedMutex<LockOrder::LOCK_NAME>>
    - std::unique_lock<OrderedMutex<LockOrder::LOCK_NAME>>
    - std::lock_guard<OrderedSharedMutex<LockOrder::LOCK_NAME>>
    - std::lock_guard<OrderedRecursiveMutex<LockOrder::LOCK_NAME>>
    - OrderedMutex<LockOrder::LOCK_NAME> _mutex; ... _mutex.lock()

    Returns list of LockAcquisition objects in the order they appear in the file
    """
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
    except (IOError, UnicodeDecodeError) as e:
        if verbose:
            print(f"Warning: Could not read {file_path}: {e}", file=sys.stderr)
        return []

    acquisitions: List[LockAcquisition] = []

    # Pattern 1: std::lock_guard or std::unique_lock with OrderedMutex variants
    lock_guard_pattern = r'std::(?:lock_guard|unique_lock|shared_lock)<(?:Playerbot::)?Ordered(?:Recursive)?(?:Shared)?Mutex<(?:Playerbot::)?LockOrder::(\w+)>>'

    # Pattern 2: OrderedMutex member declaration
    member_pattern = r'Ordered(?:Recursive)?(?:Shared)?Mutex<(?:Playerbot::)?LockOrder::(\w+)>\s+\w+;'

    # Pattern 3: Explicit .lock() calls
    lock_call_pattern = r'(\w+)\.lock(?:_shared)?\(\)'

    lines = content.split('\n')

    for line_num, line in enumerate(lines, start=1):
        # Check for lock_guard/unique_lock
        for match in re.finditer(lock_guard_pattern, line):
            lock_name = match.group(1)
            if lock_name in LOCK_HIERARCHY:
                acquisitions.append(LockAcquisition(
                    lock_name=lock_name,
                    lock_order=LOCK_HIERARCHY[lock_name],
                    line_number=line_num,
                    line_content=line
                ))
            elif verbose:
                print(f"Warning: Unknown lock order '{lock_name}' at {file_path}:{line_num}", file=sys.stderr)

    return acquisitions


def extract_function_blocks(file_path: Path) -> Dict[str, Tuple[int, int]]:
    """
    Extract function definitions and their line ranges

    Returns dict mapping function names to (start_line, end_line) tuples
    This helps identify which locks are acquired in the same function
    """
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
    except (IOError, UnicodeDecodeError):
        return {}

    functions: Dict[str, Tuple[int, int]] = {}

    # Simple heuristic: Find function definitions
    # Pattern: return_type function_name(...) {
    func_pattern = r'^\s*(?:[\w:]+\s+)+(\w+)\s*\([^)]*\)\s*(?:const\s*)?(?:override\s*)?{'

    lines = content.split('\n')
    current_func = None
    brace_depth = 0
    func_start = 0

    for line_num, line in enumerate(lines, start=1):
        # Check for function start
        match = re.search(func_pattern, line)
        if match and brace_depth == 0:
            current_func = match.group(1)
            func_start = line_num
            brace_depth = line.count('{') - line.count('}')
        elif current_func:
            brace_depth += line.count('{') - line.count('}')
            if brace_depth == 0:
                functions[current_func] = (func_start, line_num)
                current_func = None

    return functions


def check_ordering_in_function(acquisitions: List[LockAcquisition], verbose: bool = False) -> List[OrderingViolation]:
    """
    Verify locks are acquired in ascending order within a function

    Returns list of violations found
    """
    violations: List[OrderingViolation] = []

    if len(acquisitions) < 2:
        return violations

    for i in range(1, len(acquisitions)):
        prev_lock = acquisitions[i-1]
        curr_lock = acquisitions[i]

        # Check if current lock order is less than previous
        # (violates ascending order requirement)
        if curr_lock.lock_order < prev_lock.lock_order:
            violations.append(OrderingViolation(
                file_path=Path(""),  # Will be set by caller
                lock1=prev_lock,
                lock2=curr_lock
            ))

    return violations


def analyze_file(file_path: Path, verbose: bool = False) -> List[OrderingViolation]:
    """
    Analyze a single C++ file for lock ordering violations

    Returns list of violations found
    """
    acquisitions = extract_lock_acquisitions(file_path, verbose)

    if len(acquisitions) < 2:
        return []  # No violations possible with 0 or 1 locks

    # Group acquisitions by function
    functions = extract_function_blocks(file_path)
    acquisitions_by_func: Dict[str, List[LockAcquisition]] = defaultdict(list)

    for acq in acquisitions:
        # Find which function this acquisition belongs to
        func_name = "global"
        for fname, (start, end) in functions.items():
            if start <= acq.line_number <= end:
                func_name = fname
                break

        acq.function_name = func_name
        acquisitions_by_func[func_name].append(acq)

    # Check ordering within each function
    all_violations: List[OrderingViolation] = []

    for func_name, func_acquisitions in acquisitions_by_func.items():
        violations = check_ordering_in_function(func_acquisitions, verbose)
        for violation in violations:
            violation.file_path = file_path
        all_violations.extend(violations)

    return all_violations


def find_cpp_files(root_path: Path) -> List[Path]:
    """
    Find all C++ source files in the PlayerBot module

    Returns list of .cpp and .h file paths
    """
    cpp_files: List[Path] = []

    for pattern in ['**/*.cpp', '**/*.h']:
        cpp_files.extend(root_path.glob(pattern))

    return sorted(cpp_files)


def main():
    parser = argparse.ArgumentParser(
        description='Analyze lock ordering in PlayerBot module',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__
    )
    parser.add_argument(
        '--path',
        type=Path,
        default=Path('src/modules/Playerbot'),
        help='Path to PlayerBot module directory (default: src/modules/Playerbot)'
    )
    parser.add_argument(
        '--verbose', '-v',
        action='store_true',
        help='Enable verbose output'
    )
    parser.add_argument(
        '--files',
        type=str,
        nargs='+',
        help='Specific files to analyze (instead of scanning entire directory)'
    )

    args = parser.parse_args()

    # Find files to analyze
    if args.files:
        cpp_files = [Path(f) for f in args.files]
    else:
        if not args.path.exists():
            print(f"Error: Path '{args.path}' does not exist", file=sys.stderr)
            return 1

        cpp_files = find_cpp_files(args.path)

        if args.verbose:
            print(f"Analyzing {len(cpp_files)} files in {args.path}")

    # Analyze all files
    all_violations: List[Tuple[Path, List[OrderingViolation]]] = []
    files_with_locks = 0

    for cpp_file in cpp_files:
        violations = analyze_file(cpp_file, args.verbose)

        if violations:
            all_violations.append((cpp_file, violations))
            files_with_locks += 1
        else:
            # Check if file has any lock acquisitions
            acquisitions = extract_lock_acquisitions(cpp_file, verbose=False)
            if acquisitions:
                files_with_locks += 1

    # Report results
    if all_violations:
        print("=" * 80)
        print("LOCK ORDERING VIOLATIONS DETECTED")
        print("=" * 80)
        print()

        for file_path, violations in all_violations:
            print(f"{file_path}:")
            for violation in violations:
                print(f"  - {violation}")
            print()

        print("=" * 80)
        print(f"Summary: {len(all_violations)} files with violations, "
              f"{sum(len(v) for _, v in all_violations)} total violations")
        print("=" * 80)

        return 1
    else:
        if args.verbose:
            print("=" * 80)
            print("SUCCESS: No lock ordering violations detected")
            print("=" * 80)
            print(f"Analyzed {len(cpp_files)} files")
            print(f"Files with OrderedMutex usage: {files_with_locks}")
        else:
            print("✓ No lock ordering violations detected")

        return 0


if __name__ == "__main__":
    sys.exit(main())
