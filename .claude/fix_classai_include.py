#!/usr/bin/env python3
"""
Fix ClassAI.cpp include issue
Adds SpellPacketBuilder.h include in the correct location
"""

def fix_classai_include():
    file_path = "c:/TrinityBots/TrinityCore/src/modules/Playerbot/AI/ClassAI/ClassAI.cpp"

    print(f"Reading {file_path}...")
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()

    # Find line 36 (GridNotifiersImpl.h) and add our include after it
    old_includes = '#include "GridNotifiersImpl.h"\n\nnamespace Playerbot'
    new_includes = '#include "GridNotifiersImpl.h"\n#include "../../Packets/SpellPacketBuilder.h"  // PHASE 0 WEEK 3: Packet-based spell casting\n\nnamespace Playerbot'

    content = content.replace(old_includes, new_includes)

    print(f"Writing fixed code to {file_path}...")
    with open(file_path, 'w', encoding='utf-8', newline='\n') as f:
        f.write(content)

    print("OK - Added SpellPacketBuilder.h include")
    return True

if __name__ == "__main__":
    import sys
    success = fix_classai_include()
    sys.exit(0 if success else 1)
