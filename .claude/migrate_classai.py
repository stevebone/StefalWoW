#!/usr/bin/env python3
"""
ClassAI.cpp Packet-Based Migration Script
PHASE 0 WEEK 3 PHASE 2 (2025-10-30)

Migrates ClassAI.cpp wrapper methods from direct CastSpell() API calls to
packet-based spell casting using SpellPacketBuilder.

This affects ALL 39 class specializations indirectly (they all call ClassAI::CastSpell).

Changes:
1. Add SpellPacketBuilder.h include
2. Replace CastSpell(Unit*, uint32) with packet-based implementation
3. CastSpell(uint32) self-cast version remains unchanged (calls above)
"""

import re
import sys

def migrate_classai():
    file_path = "c:/TrinityBots/TrinityCore/src/modules/Playerbot/AI/ClassAI/ClassAI.cpp"

    print(f"Reading {file_path}...")
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()

    original_content = content

    # Fix 1: Check if SpellPacketBuilder.h is already included
    if '#include "../../Packets/SpellPacketBuilder.h"' not in content:
        # Find the last #include before namespace
        # Look for pattern: #include "..." followed by empty line and namespace
        pattern = r'(#include "[^"]+"\n)(namespace Playerbot)'
        replacement = r'\1#include "../../Packets/SpellPacketBuilder.h"  // PHASE 0 WEEK 3: Packet-based spell casting\n\n\2'
        content = re.sub(pattern, replacement, content, count=1)
        print("Added SpellPacketBuilder.h include")
    else:
        print("SpellPacketBuilder.h already included, skipping")

    # Fix 2: Replace ClassAI::CastSpell(Unit* target, uint32 spellId) method
    old_castspell = r'''bool ClassAI::CastSpell\(::Unit\* target, uint32 spellId\)
\{
    if \(!target \|\| !spellId \|\| !GetBot\(\)\)
        return false;

    if \(!IsSpellUsable\(spellId\)\)
        return false;

    if \(!IsInRange\(target, spellId\)\)
        return false;

    if \(!HasLineOfSight\(target\)\)
        return false;

    // Cast the spell
    SpellInfo const\* spellInfo = sSpellMgr->GetSpellInfo\(spellId, DIFFICULTY_NONE\);
    if \(!spellInfo\)
        return false;

    GetBot\(\)->CastSpell\(target, spellId, false\);
    ConsumeResource\(spellId\);
    _cooldownManager->StartCooldown\(spellId, spellInfo->RecoveryTime\);

    return true;
\}'''

    new_castspell = '''bool ClassAI::CastSpell(::Unit* target, uint32 spellId)
{
    if (!target || !spellId || !GetBot())
        return false;

    // MIGRATION COMPLETE (2025-10-30):
    // Replaced direct CastSpell() API call with packet-based SpellPacketBuilder.
    // BEFORE: GetBot()->CastSpell(target, spellId, false); // UNSAFE - worker thread
    // AFTER: SpellPacketBuilder::BuildCastSpellPacket(...) // SAFE - queues to main thread
    // IMPACT: All 39 class specializations now use thread-safe spell casting

    // Pre-validation (ClassAI-specific checks before packet building)
    if (!IsSpellUsable(spellId))
    {
        TC_LOG_TRACE("playerbot.classai.spell",
                     "ClassAI spell {} not usable for bot {}",
                     spellId, GetBot()->GetName());
        return false;
    }

    if (!IsInRange(target, spellId))
    {
        TC_LOG_TRACE("playerbot.classai.spell",
                     "ClassAI spell {} target out of range for bot {}",
                     spellId, GetBot()->GetName());
        return false;
    }

    if (!HasLineOfSight(target))
    {
        TC_LOG_TRACE("playerbot.classai.spell",
                     "ClassAI spell {} target no LOS for bot {}",
                     spellId, GetBot()->GetName());
        return false;
    }

    // Get spell info for validation and cooldown tracking
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
    {
        TC_LOG_TRACE("playerbot.classai.spell",
                     "ClassAI spell {} not found in spell data for bot {}",
                     spellId, GetBot()->GetName());
        return false;
    }

    // Build packet with validation
    SpellPacketBuilder::BuildOptions options;
    options.skipGcdCheck = false;      // Respect GCD
    options.skipResourceCheck = false; // Check mana/energy/rage
    options.skipTargetCheck = false;   // Check target validity
    options.skipStateCheck = false;    // Check caster state
    options.skipRangeCheck = false;    // Check spell range (double-check after ClassAI check)
    options.logFailures = true;        // Log validation failures

    auto result = SpellPacketBuilder::BuildCastSpellPacket(GetBot(), spellId, target, options);

    if (result.result == SpellPacketBuilder::ValidationResult::SUCCESS)
    {
        // Packet successfully queued to main thread

        // Optimistic resource consumption and cooldown tracking
        // (Will be validated again on main thread, but tracking here for ClassAI responsiveness)
        ConsumeResource(spellId);
        _cooldownManager->StartCooldown(spellId, spellInfo->RecoveryTime);

        TC_LOG_DEBUG("playerbot.classai.spell",
                     "ClassAI queued CMSG_CAST_SPELL for spell {} (bot: {}, target: {})",
                     spellId, GetBot()->GetName(), target->GetName());
        return true;
    }
    else
    {
        // Validation failed - packet not queued
        TC_LOG_TRACE("playerbot.classai.spell",
                     "ClassAI spell {} validation failed for bot {}: {} ({})",
                     spellId, GetBot()->GetName(),
                     static_cast<uint8>(result.result),
                     result.failureReason);
        return false;
    }
}'''

    content = re.sub(old_castspell, new_castspell, content, flags=re.DOTALL, count=1)

    # Verify changes were applied
    if content == original_content:
        print("ERROR: No changes were applied - pattern matching failed")
        return False

    # Write modified content
    print(f"Writing migrated code to {file_path}...")
    with open(file_path, 'w', encoding='utf-8', newline='\n') as f:
        f.write(content)

    print("OK - ClassAI.cpp migration complete")
    print("")
    print("Changes applied:")
    print("1. Added #include for SpellPacketBuilder.h (if not present)")
    print("2. Migrated CastSpell(Unit*, uint32) to use SpellPacketBuilder::BuildCastSpellPacket()")
    print("3. Added comprehensive validation result checking")
    print("4. Added DEBUG/TRACE logging for packet queue operations")
    print("5. Optimistic resource consumption and cooldown tracking")
    print("")
    print("Impact: ALL 39 class specializations now use packet-based spell casting")
    print("Next step: Compile to verify migration (expect 0 errors)")
    return True

if __name__ == "__main__":
    success = migrate_classai()
    sys.exit(0 if success else 1)
