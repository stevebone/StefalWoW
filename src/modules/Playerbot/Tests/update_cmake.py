#!/usr/bin/env python3
"""Generate CMakeLists.txt additions"""
import os
from pathlib import Path

test_files = []

# Find all test .cpp files
for root, dirs, files in os.walk('.'):
    for file in files:
        if file.endswith('Test.cpp') and 'Phase3' not in root:
            rel_path = os.path.join(root, file).replace('./', '').replace('\\', '/')
            test_files.append(rel_path)

test_files.sort()

print("# ============================================================================")
print("# COMPREHENSIVE TEST SUITE - 585+ TESTS")
print("# ============================================================================")
print()
print("set(PLAYERBOT_COMPREHENSIVE_TEST_SOURCES")
print("    # Test Helpers and Utilities")
print("    TestHelpers.h")
print()
print("    # Phase 3 Mock Framework")
print("    Phase3/Unit/CustomMain.cpp")
print("    Phase3/Unit/SimpleTest.cpp")
print("    Phase3/Unit/Mocks/MockFramework.cpp")
print()
print("    # ClassAI Tests (585 tests across 39 specs)")
for f in test_files:
    if 'ClassAI' in f:
        print(f"    {f}")
print()
print("    # Combat System Tests (40 tests)")
for f in test_files:
    if 'Combat' in f and 'ClassAI' not in f:
        print(f"    {f}")
print()
print("    # Lifecycle Tests (30 tests)")
for f in test_files:
    if 'Lifecycle' in f:
        print(f"    {f}")
print()
print("    # Integration Tests (40 tests)")
for f in test_files:
    if 'Integration' in f:
        print(f"    {f}")
print(")")
print()
print(f"# Total test files: {len(test_files) + 3}")
