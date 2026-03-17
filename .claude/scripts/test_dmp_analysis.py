#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Test DMP analysis with CDB integration
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

from crash_analyzer import DmpAnalyzer

def main():
    print("=" * 80)
    print("🧪 Testing DMP Analysis with CDB")
    print("=" * 80)

    crashes_dir = Path("M:/Wplayerbot/Crashes")
    pdb_dir = Path("M:/Wplayerbot")  # Directory containing worldserver.pdb

    # Find most recent .dmp file
    dmp_files = sorted(crashes_dir.glob("*.dmp"), key=lambda p: p.stat().st_mtime, reverse=True)

    if not dmp_files:
        print("❌ No .dmp files found in", crashes_dir)
        return 1

    dmp_file = dmp_files[0]
    print(f"\n📄 Testing with: {dmp_file.name}")
    print(f"   Size: {dmp_file.stat().st_size:,} bytes")
    print(f"   Modified: {dmp_file.stat().st_mtime}")

    # Create analyzer
    print(f"\n🔧 Initializing DmpAnalyzer...")
    print(f"   PDB Directory: {pdb_dir}")
    analyzer = DmpAnalyzer(pdb_dir=pdb_dir)

    if not analyzer.cdb_available:
        print(f"❌ CDB not found at: {analyzer.cdb_path}")
        print("   Please install Windows Debugging Tools")
        return 1

    print(f"   ✅ CDB found: {analyzer.cdb_path}")

    # Analyze dump
    print(f"\n🔍 Analyzing .dmp file with CDB...")
    print(f"   This may take 30-60 seconds...")

    result = analyzer.analyze(dmp_file)

    if not result:
        print("❌ Analysis failed")
        return 1

    # Print results
    print("\n" + "=" * 80)
    print("✅ DMP ANALYSIS RESULTS")
    print("=" * 80)

    print(f"\n💥 Exception Information:")
    print(f"   Code: {result.get('exception_code', 'N/A')}")
    print(f"   Description: {result.get('exception_description', 'N/A')}")
    print(f"   Fault Address: {result.get('fault_address', 'N/A')}")
    print(f"   Faulting Module: {result.get('faulting_module', 'N/A')}")
    print(f"   Faulting Function: {result.get('faulting_function', 'N/A')}")

    print(f"\n📊 CPU Registers (ALL {len(result.get('registers', {}))} registers):")
    registers = result.get('registers', {})
    if registers:
        # Sort registers: general purpose first, then segment, then flags
        reg_order = ['rax', 'rbx', 'rcx', 'rdx', 'rsi', 'rdi', 'rbp', 'rsp', 'rip',
                     'r8', 'r9', 'r10', 'r11', 'r12', 'r13', 'r14', 'r15',
                     'cs', 'ss', 'ds', 'es', 'fs', 'gs', 'efl', 'iopl']

        displayed = set()
        for reg in reg_order:
            if reg in registers:
                val = registers[reg]
                # Highlight null pointers
                if val == '0000000000000000' or val == '00000000':
                    print(f"   {reg:5} = {val} ⚠️  NULL")
                else:
                    print(f"   {reg:5} = {val}")
                displayed.add(reg)

        # Show any remaining registers not in order
        for reg, val in registers.items():
            if reg not in displayed:
                print(f"   {reg:5} = {val}")
    else:
        print("   No register data available")

    print(f"\n📚 Call Stack (Detailed) - FULL STACK:")
    call_stack = result.get('call_stack_detailed', [])
    if call_stack:
        for i, frame in enumerate(call_stack, 1):  # Show ALL frames
            print(f"   {i:2}. {frame}")
        print(f"\n   Total frames: {len(call_stack)}")
    else:
        print("   No call stack data available")

    print(f"\n🔍 Local Variables:")
    local_vars = result.get('local_variables', {})
    if local_vars:
        for var, val in list(local_vars.items())[:10]:  # Show first 10
            # Highlight null pointers
            if 'null' in val.lower() or val == '0x0' or val == '0':
                print(f"   {var:20} = {val} ⚠️  NULL POINTER")
            else:
                print(f"   {var:20} = {val}")
        if len(local_vars) > 10:
            print(f"   ... and {len(local_vars) - 10} more variables")
    else:
        print("   No local variable data available")

    print(f"\n🛡️  Heap Analysis:")
    if result.get('heap_corruption', False):
        print("   ⚠️  HEAP CORRUPTION DETECTED")
        print("   This is likely a use-after-free or double-free bug")
    else:
        print("   ✅ No heap corruption detected")

    print(f"\n💡 CDB Recommendations:")
    recommendations = result.get('recommendations', [])
    if recommendations:
        for i, rec in enumerate(recommendations, 1):
            print(f"   {i}. {rec}")
    else:
        print("   No specific recommendations from CDB")

    print("\n" + "=" * 80)
    print("✅ TEST COMPLETE")
    print("=" * 80)

    # Validation summary
    print("\n🎯 Validation:")
    validation_passed = 0
    validation_total = 5

    if result.get('exception_code') and result['exception_code'] != 'UNKNOWN':
        print("   ✅ Exception code parsed correctly")
        validation_passed += 1
    else:
        print("   ❌ Exception code not available")

    if result.get('fault_address') and result['fault_address'] != 'UNKNOWN':
        print("   ✅ Fault address parsed correctly")
        validation_passed += 1
    else:
        print("   ❌ Fault address not available")

    if result.get('registers'):
        print(f"   ✅ Register dump captured ({len(result['registers'])} registers)")
        validation_passed += 1
    else:
        print("   ❌ No register data")

    if result.get('call_stack_detailed'):
        print(f"   ✅ Detailed call stack captured ({len(result['call_stack_detailed'])} frames)")
        validation_passed += 1
    else:
        print("   ❌ No call stack data")

    if result.get('recommendations'):
        print(f"   ✅ CDB recommendations available ({len(result['recommendations'])} items)")
        validation_passed += 1
    else:
        print("   ⚠️  No CDB recommendations")

    print(f"\n📊 Validation Score: {validation_passed}/{validation_total} ({validation_passed/validation_total*100:.0f}%)")

    if validation_passed >= 3:
        print("   ✅ DMP analysis is working correctly!")
        return 0
    else:
        print("   ⚠️  DMP analysis may need symbol path configuration")
        return 1

if __name__ == "__main__":
    sys.exit(main())
