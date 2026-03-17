#!/usr/bin/env python3
"""
BaselineRotationManager.cpp Packet-Based Migration Script
PHASE 0 WEEK 3 (2025-10-30)

Migrates BaselineRotationManager.cpp from direct CastSpell() API calls to
packet-based spell casting using SpellPacketBuilder.

Changes:
1. Add SpellPacketBuilder.h include
2. Replace TryCastAbility() CastSpell() with BuildCastSpellPacket()
3. Add comprehensive validation and logging
"""

import re
import sys

def migrate_baseline_rotation():
    file_path = "c:/TrinityBots/TrinityCore/src/modules/Playerbot/AI/ClassAI/BaselineRotationManager.cpp"

    print(f"Reading {file_path}...")
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()

    original_content = content

    # Fix 1: Update file header comment
    content = re.sub(
        r'/\*\n \* Copyright \(C\) 2024 TrinityCore <https://www.trinitycore.org/>\n \*\n \* Baseline Rotation Manager Implementation - FIXED VERSION\n \*/',
        r'/*\n * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>\n *\n * Baseline Rotation Manager Implementation - PACKET-BASED MIGRATION\n * PHASE 0 WEEK 3 (2025-10-30): Migrated to packet-based spell casting\n */',
        content,
        count=1
    )

    # Fix 2: Add SpellPacketBuilder.h include after existing includes
    content = re.sub(
        r'(#include "Log\.h"  // For TC_LOG_DEBUG\n#include "\.\./\.\./Spatial/SpatialGridQueryHelpers\.h"  // PHASE 5F: Thread-safe queries)',
        r'\1\n#include "../../Packets/SpellPacketBuilder.h"  // PHASE 0 WEEK 3: Packet-based spell casting',
        content,
        count=1
    )

    # Fix 3: Replace TryCastAbility method with packet-based implementation
    old_try_cast = r'''bool BaselineRotationManager::TryCastAbility\(Player\* bot, ::Unit\* target, BaselineAbility const& ability\)
\{
    if \(!CanUseAbility\(bot, target, ability\)\)
        return false;

    // Check cooldown
    auto& botCooldowns = _cooldowns\[bot->GetGUID\(\)\.GetCounter\(\)\];
    auto cdIt = botCooldowns\.find\(ability\.spellId\);
    if \(cdIt != botCooldowns\.end\(\) && cdIt->second > getMSTime\(\)\)
        return false; // On cooldown

    // Cast spell
    // FIX: GetSpellInfo requires difficulty parameter
    SpellInfo const\* spellInfo = sSpellMgr->GetSpellInfo\(ability\.spellId, bot->GetMap\(\)->GetDifficultyID\(\)\);
    if \(!spellInfo\)
        return false;

    // Determine cast target
    ::Unit\* castTarget = ability\.requiresMelee \? target : \(ability\.isDefensive \? bot : target\);

    // Cast spell using TrinityCore API
    if \(bot->CastSpell\(castTarget, ability\.spellId, false\)\)
    \{
        // Record cooldown
        botCooldowns\[ability\.spellId\] = getMSTime\(\) \+ ability\.cooldown;
        return true;
    \}

    return false;
\}'''

    new_try_cast = '''bool BaselineRotationManager::TryCastAbility(Player* bot, ::Unit* target, BaselineAbility const& ability)
{
    if (!CanUseAbility(bot, target, ability))
        return false;

    // Check cooldown
    auto& botCooldowns = _cooldowns[bot->GetGUID().GetCounter()];
    auto cdIt = botCooldowns.find(ability.spellId);
    if (cdIt != botCooldowns.end() && cdIt->second > getMSTime())
        return false; // On cooldown

    // MIGRATION COMPLETE (2025-10-30):
    // Replaced direct CastSpell() API call with packet-based SpellPacketBuilder.
    // BEFORE: bot->CastSpell(castTarget, spellId, false); // UNSAFE - worker thread
    // AFTER: SpellPacketBuilder::BuildCastSpellPacket(...) // SAFE - queues to main thread

    // Get spell info for validation
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(ability.spellId, bot->GetMap()->GetDifficultyID());
    if (!spellInfo)
    {
        TC_LOG_TRACE("playerbot.baseline.packets",
                     "Bot {} spell {} not found in spell data",
                     bot->GetName(), ability.spellId);
        return false;
    }

    // Determine cast target
    ::Unit* castTarget = ability.requiresMelee ? target : (ability.isDefensive ? bot : target);

    // Build packet with validation
    SpellPacketBuilder::BuildOptions options;
    options.skipGcdCheck = false;      // Respect GCD
    options.skipResourceCheck = false; // Check mana/energy/rage
    options.skipRangeCheck = false;    // Check spell range
    options.skipCastTimeCheck = false; // Check cast time vs movement
    options.skipCooldownCheck = false; // Check cooldowns (double-check)
    options.skipLosCheck = false;      // Check line of sight

    auto result = SpellPacketBuilder::BuildCastSpellPacket(bot, ability.spellId, castTarget, options);

    if (result.result == SpellPacketBuilder::ValidationResult::SUCCESS)
    {
        // Packet successfully queued to main thread

        // Optimistic cooldown recording (packet will be processed)
        botCooldowns[ability.spellId] = getMSTime() + ability.cooldown;

        TC_LOG_DEBUG("playerbot.baseline.packets",
                     "Bot {} queued CMSG_CAST_SPELL for baseline spell {} (target: {})",
                     bot->GetName(), ability.spellId,
                     castTarget ? castTarget->GetName() : "self");
        return true;
    }
    else
    {
        // Validation failed - packet not queued
        TC_LOG_TRACE("playerbot.baseline.packets",
                     "Bot {} baseline spell {} validation failed: {} ({})",
                     bot->GetName(), ability.spellId,
                     static_cast<uint8>(result.result),
                     result.errorMessage);
        return false;
    }
}'''

    content = re.sub(old_try_cast, new_try_cast, content, flags=re.DOTALL, count=1)

    # Verify changes were applied
    if content == original_content:
        print("ERROR: No changes were applied - pattern matching failed")
        return False

    # Write modified content
    print(f"Writing migrated code to {file_path}...")
    with open(file_path, 'w', encoding='utf-8', newline='\n') as f:
        f.write(content)

    print("OK - BaselineRotationManager.cpp migration complete")
    print("")
    print("Changes applied:")
    print("1. Updated file header to indicate packet-based migration")
    print("2. Added #include for SpellPacketBuilder.h")
    print("3. Migrated TryCastAbility() to use SpellPacketBuilder::BuildCastSpellPacket()")
    print("4. Added comprehensive validation result checking")
    print("5. Added DEBUG/TRACE logging for packet queue operations")
    print("")
    print("Next step: Compile to verify migration (expect 0 errors)")
    return True

if __name__ == "__main__":
    success = migrate_baseline_rotation()
    sys.exit(0 if success else 1)
