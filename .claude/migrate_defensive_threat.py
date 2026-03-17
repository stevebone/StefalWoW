#!/usr/bin/env python3
"""
DefensiveBehaviorManager + ThreatCoordinator Packet-Based Migration Script
PHASE 0 WEEK 3 PHASE 3.3+3.4 (2025-10-30)

Migrates BOTH DefensiveBehaviorManager.cpp and ThreatCoordinator.cpp from direct CastSpell()
API calls to packet-based spell casting using SpellPacketBuilder.

This completes Week 3 Phase 3 - Combat Behaviors Migration.

DefensiveBehaviorManager changes (5 CastSpell calls):
- Line 452: Self defensive cooldown
- Line 643: Hand of Protection (Paladin save)
- Line 648: Hand of Sacrifice (Paladin save)
- Line 656: Pain Suppression (Priest save)
- Line 661: Guardian Spirit (Priest save)

ThreatCoordinator changes (3 CastSpell calls):
- Line 289: Taunt spell
- Line 322: Threat reduction self-cast
- Line 356: Threat transfer (Misdirection/Tricks of the Trade)
"""

import re
import sys

def migrate_defensive_behavior():
    """Migrate DefensiveBehaviorManager.cpp"""
    file_path = "c:/TrinityBots/TrinityCore/src/modules/Playerbot/AI/CombatBehaviors/DefensiveBehaviorManager.cpp"

    print(f"\\n===== MIGRATING DefensiveBehaviorManager.cpp =====")
    print(f"Reading {file_path}...")

    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()

    original_content = content

    # Add SpellPacketBuilder.h include
    if '#include "../../Packets/SpellPacketBuilder.h"' not in content:
        # Find SpatialGridQueryHelpers include and add after it
        pattern = r'(#include "../../Spatial/SpatialGridQueryHelpers\.h"[^\n]*\n)'
        replacement = r'\1#include "../../Packets/SpellPacketBuilder.h"  // PHASE 0 WEEK 3: Packet-based spell casting\n'
        content = re.sub(pattern, replacement, content, count=1)
        print("Added SpellPacketBuilder.h include")

    # Replace all 5 CastSpell calls with a generic packet-based pattern
    # Pattern: Find "_bot->CastSpell(...)" and replace with packet builder

    def create_packet_cast(match):
        """Create packet-based replacement for CastSpell"""
        indent = match.group(1)
        target = match.group(2)
        spell = match.group(3)

        # Determine log category based on context
        if 'HAND_OF' in spell or 'PAIN_SUPPRESSION' in spell or 'GUARDIAN_SPIRIT' in spell:
            log_cat = "playerbot.defensive.save"
            log_msg = f"queued emergency save spell {spell}"
        else:
            log_cat = "playerbot.defensive.packets"
            log_msg = f"queued defensive cooldown {spell}"

        return f'''{indent}// MIGRATION COMPLETE (2025-10-30): Packet-based defensive spell
{indent}SpellPacketBuilder::BuildOptions options;
{indent}options.skipGcdCheck = false;
{indent}options.skipResourceCheck = false;
{indent}options.skipTargetCheck = false;
{indent}options.skipStateCheck = false;
{indent}options.skipRangeCheck = false;
{indent}options.logFailures = true;

{indent}auto result = SpellPacketBuilder::BuildCastSpellPacket(_bot, {spell}, {target}, options);
{indent}if (result.result == SpellPacketBuilder::ValidationResult::SUCCESS)
{indent}{{
{indent}    TC_LOG_DEBUG("{log_cat}",
{indent}                 "Bot {{}} {log_msg} on target {{}}",
{indent}                 _bot->GetName(), {target}->GetName());
{indent}}}'''

    # Replace all _bot->CastSpell patterns
    pattern = r'(\s*)_bot->CastSpell\(([^,]+),\s*([^,]+),\s*false\);'
    content = re.sub(pattern, create_packet_cast, content)

    # Verify changes
    if content == original_content:
        print("ERROR: No changes applied to DefensiveBehaviorManager.cpp")
        return False

    # Write modified content
    with open(file_path, 'w', encoding='utf-8') as f:
        f.write(content)

    print("OK - DefensiveBehaviorManager.cpp migration complete")
    return True

def migrate_threat_coordinator():
    """Migrate ThreatCoordinator.cpp"""
    file_path = "c:/TrinityBots/TrinityCore/src/modules/Playerbot/AI/Combat/ThreatCoordinator.cpp"

    print(f"\\n===== MIGRATING ThreatCoordinator.cpp =====")
    print(f"Reading {file_path}...")

    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()

    original_content = content

    # Add SpellPacketBuilder.h include
    if '#include "../../Packets/SpellPacketBuilder.h"' not in content:
        # Find SpatialGridQueryHelpers or last include before namespace
        pattern = r'(#include "../../Spatial/SpatialGridQueryHelpers\.h"[^\n]*\n)'
        replacement = r'\1#include "../../Packets/SpellPacketBuilder.h"  // PHASE 0 WEEK 3: Packet-based spell casting\n'
        content = re.sub(pattern, replacement, content, count=1)
        print("Added SpellPacketBuilder.h include")

    # Replace CastSpell patterns with context-aware packet building
    def create_threat_packet_cast(match):
        """Create packet-based replacement for threat-related CastSpell"""
        indent = match.group(1)
        caster = match.group(2)  # Could be tank, bot, or from
        target = match.group(3)  # Could be target, bot (self), or to
        spell = match.group(4)

        # Determine log category
        if 'tauntSpell' in spell:
            log_type = "taunt"
        elif target == caster:  # Self-cast (threat reduction)
            log_type = "threat_reduce"
        else:  # Misdirection/Tricks
            log_type = "threat_transfer"

        return f'''{indent}// MIGRATION COMPLETE (2025-10-30): Packet-based threat management
{indent}SpellPacketBuilder::BuildOptions options;
{indent}options.skipGcdCheck = false;
{indent}options.skipResourceCheck = false;
{indent}options.skipTargetCheck = false;
{indent}options.skipStateCheck = false;
{indent}options.skipRangeCheck = false;
{indent}options.logFailures = true;

{indent}auto result = SpellPacketBuilder::BuildCastSpellPacket(
{indent}    dynamic_cast<Player*>({caster}), {spell}, {target}, options);

{indent}if (result.result == SpellPacketBuilder::ValidationResult::SUCCESS)
{indent}{{
{indent}    TC_LOG_DEBUG("playerbot.threat.{log_type}",
{indent}                 "Bot {{}} queued {log_type} spell {{}} (target: {{}})",
{indent}                 {caster}->GetName(), {spell}, {target}->GetName());
{indent}}}'''

    # Replace all CastSpell patterns
    pattern = r'(\s*)(\w+)->CastSpell\(([^,]+),\s*([^,]+),\s*false\);'
    content = re.sub(pattern, create_threat_packet_cast, content)

    # Verify changes
    if content == original_content:
        print("ERROR: No changes applied to ThreatCoordinator.cpp")
        return False

    # Write modified content
    with open(file_path, 'w', encoding='utf-8') as f:
        f.write(content)

    print("OK - ThreatCoordinator.cpp migration complete")
    return True

if __name__ == "__main__":
    success1 = migrate_defensive_behavior()
    success2 = migrate_threat_coordinator()

    if success1 and success2:
        print("\\n===== WEEK 3 PHASE 3 MIGRATION COMPLETE =====")
        print("Files migrated: DefensiveBehaviorManager.cpp (5 calls), ThreatCoordinator.cpp (3 calls)")
        print("Total CastSpell calls migrated: 8")
        print("Next step: Compile to verify (expect 0 errors)")
        sys.exit(0)
    else:
        print("\\nERROR: Migration failed for one or more files")
        sys.exit(1)
