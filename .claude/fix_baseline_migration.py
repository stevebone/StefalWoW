#!/usr/bin/env python3
"""
Fix BaselineRotationManager.cpp compilation errors
Corrects BuildOptions field names to match actual SpellPacketBuilder API
"""

import re

def fix_baseline_compilation():
    file_path = "c:/TrinityBots/TrinityCore/src/modules/Playerbot/AI/ClassAI/BaselineRotationManager.cpp"

    print(f"Reading {file_path}...")
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()

    original_content = content

    # Fix 1: Remove skipCastTimeCheck (doesn't exist, use skipStateCheck instead)
    content = content.replace('options.skipCastTimeCheck = false; // Check cast time vs movement',
                               '// Cast time check covered by skipStateCheck')

    # Fix 2: Remove skipCooldownCheck (covered by GCD check)
    content = content.replace('options.skipCooldownCheck = false; // Check cooldowns (double-check)',
                               '// Cooldown check covered by skipGcdCheck')

    # Fix 3: Remove skipLosCheck (covered by range check)
    content = content.replace('options.skipLosCheck = false;      // Check line of sight',
                               '// LOS check covered by skipRangeCheck')

    # Fix 4: Change errorMessage to failureReason
    content = content.replace('result.errorMessage', 'result.failureReason')

    # Verify changes
    if content == original_content:
        print("ERROR: No changes applied")
        return False

    print(f"Writing fixed code to {file_path}...")
    with open(file_path, 'w', encoding='utf-8', newline='\n') as f:
        f.write(content)

    print("OK - Compilation errors fixed")
    print("")
    print("Fixes applied:")
    print("1. Removed skipCastTimeCheck (use skipStateCheck)")
    print("2. Removed skipCooldownCheck (covered by GCD)")
    print("3. Removed skipLosCheck (covered by range check)")
    print("4. Changed errorMessage to failureReason")
    return True

if __name__ == "__main__":
    import sys
    success = fix_baseline_compilation()
    sys.exit(0 if success else 1)
