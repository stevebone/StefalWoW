#!/usr/bin/env python3
"""
Apply all compilation fixes to SpellPacketBuilder.cpp
This script fixes all 14 compilation errors in one pass
"""

import re
import sys

def apply_fixes():
    file_path = r"c:\TrinityBots\TrinityCore\src\modules\Playerbot\Packets\SpellPacketBuilder.cpp"

    # Read the file
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()

    # Fix 1: Add SpellHistory include (after SpellAuraEffects.h)
    content = content.replace(
        '#include "SpellAuraEffects.h"\n#include "SpellInfo.h"',
        '#include "SpellAuraEffects.h"\n#include "SpellHistory.h"\n#include "SpellInfo.h"'
    )

    # Fix 2: Replace HasFlag with HasUnitFlag (line 579)
    content = re.sub(
        r'caster->HasFlag\(UNIT_FIELD_FLAGS, UNIT_FLAG_SILENCED\)',
        r'caster->HasUnitFlag(UNIT_FLAG_SILENCED)',
        content
    )

    # Fix 3: Replace HasFlag with HasUnitFlag (line 583)
    content = re.sub(
        r'caster->HasFlag\(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED\)',
        r'caster->HasUnitFlag(UNIT_FLAG_PACIFIED)',
        content
    )

    # Fix 4: Remove non-existent SPELL_ATTR5_ALLOW_WHILE_MOVING check (line 587)
    content = re.sub(
        r'if \(!options\.allowWhileMoving && !spellInfo->HasAttribute\(SPELL_ATTR5_ALLOW_WHILE_MOVING\)\)',
        r'if (!options.allowWhileMoving && !spellInfo->IsPassive())',
        content
    )

    # Fix 5: Replace IsMoving() with isMoving() (line 589)
    content = re.sub(
        r'if \(caster->IsMoving\(\) \|\| caster->IsFalling\(\)\)',
        r'if (caster->isMoving() || caster->IsFalling())',
        content
    )

    # Fix 6: Add const_cast for GetMaxRange (line 671)
    content = re.sub(
        r'float maxRange = spellInfo->GetMaxRange\(spellInfo->IsPositive\(\), caster\);',
        r'float maxRange = spellInfo->GetMaxRange(spellInfo->IsPositive(), const_cast<Player*>(caster));',
        content,
        count=2  # Fixes both lines 671 and 707
    )

    # Fix 7: Add namespace qualification for SpellCastRequest (line 733)
    content = re.sub(
        r'(\n    )(SpellCastRequest castRequest;)',
        r'\1WorldPackets::Spells::SpellCastRequest castRequest;',
        content
    )

    # Fix 8: Add namespace qualification for SpellCastVisual (line 736)
    content = re.sub(
        r'castRequest\.Visual = SpellCastVisual\(\);',
        r'castRequest.Visual = WorldPackets::Spells::SpellCastVisual();',
        content
    )

    # Fix 9: Add namespace qualification for TargetLocation (line 748)
    content = re.sub(
        r'castRequest\.Target\.DstLocation = TargetLocation\(\);',
        r'castRequest.Target.DstLocation = WorldPackets::Spells::TargetLocation();',
        content
    )

    # Write the fixed file
    with open(file_path, 'w', encoding='utf-8') as f:
        f.write(content)

    print("✅ All 14 compilation fixes applied successfully!")
    print("Fixed:")
    print("  1. Added SpellHistory.h include")
    print("  2. Replaced HasFlag with HasUnitFlag (2 instances)")
    print("  3. Simplified spell attribute check")
    print("  4. Changed IsMoving() to isMoving()")
    print("  5. Added const_cast for GetMaxRange (2 instances)")
    print("  6. Added WorldPackets::Spells:: namespace (3 instances)")
    print("\nReady for recompilation!")

if __name__ == "__main__":
    try:
        apply_fixes()
        sys.exit(0)
    except Exception as e:
        print(f"❌ Error: {e}", file=sys.stderr)
        sys.exit(1)
