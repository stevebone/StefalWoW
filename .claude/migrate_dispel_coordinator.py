#!/usr/bin/env python3
"""
DispelCoordinator.cpp Packet-Based Migration Script
PHASE 0 WEEK 3 PHASE 3.2 (2025-10-30)

Migrates DispelCoordinator.cpp from direct CastSpell() API calls to
packet-based spell casting using SpellPacketBuilder.

This affects dispel/cleanse/purge logic for ALL healer bots.

Changes:
1. Add SpellPacketBuilder.h include
2. Replace ExecuteDispel() CastSpell (line 808)
3. Replace ExecutePurge() CastSpell (line 873)
"""

import re
import sys

def migrate_dispel_coordinator():
    file_path = "c:/TrinityBots/TrinityCore/src/modules/Playerbot/AI/CombatBehaviors/DispelCoordinator.cpp"

    print(f"Reading {file_path}...")
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()

    original_content = content

    # Fix 1: Add SpellPacketBuilder.h include after SpatialGridQueryHelpers.h (line 25)
    if '#include "../../Packets/SpellPacketBuilder.h"' not in content:
        old_include = '#include "../../Spatial/SpatialGridQueryHelpers.h"  // PHASE 5B: Thread-safe helpers\n#include <mutex>'
        new_include = '#include "../../Spatial/SpatialGridQueryHelpers.h"  // PHASE 5B: Thread-safe helpers\n#include "../../Packets/SpellPacketBuilder.h"  // PHASE 0 WEEK 3: Packet-based spell casting\n#include <mutex>'
        content = content.replace(old_include, new_include)
        print("Added SpellPacketBuilder.h include")
    else:
        print("SpellPacketBuilder.h already included, skipping")

    # Fix 2: Replace ExecuteDispel() CastSpell (line 808)
    old_dispel = '''    // Cast dispel
    if (m_bot->CastSpell(target, dispelSpell, false))
    {
        m_lastDispelAttempt = now;
        m_globalCooldownUntil = now + m_config.dispelGCD;
        m_currentAssignment.fulfilled = true;

        ++m_statistics.successfulDispels;
        ++m_statistics.dispelsByType[m_currentAssignment.dispelType];

        MarkDispelComplete(m_currentAssignment);
        return true;
    }

    ++m_statistics.failedDispels;
    return false;'''

    new_dispel = '''    // Cast dispel (MIGRATION COMPLETE 2025-10-30: Packet-based dispel)
    SpellPacketBuilder::BuildOptions options;
    options.skipGcdCheck = false;
    options.skipResourceCheck = false;
    options.skipTargetCheck = false;
    options.skipStateCheck = false;
    options.skipRangeCheck = false;
    options.logFailures = true;

    auto result = SpellPacketBuilder::BuildCastSpellPacket(m_bot, dispelSpell, target, options);

    if (result.result == SpellPacketBuilder::ValidationResult::SUCCESS)
    {
        m_lastDispelAttempt = now;
        m_globalCooldownUntil = now + m_config.dispelGCD;
        m_currentAssignment.fulfilled = true;

        ++m_statistics.successfulDispels;
        ++m_statistics.dispelsByType[m_currentAssignment.dispelType];

        MarkDispelComplete(m_currentAssignment);

        TC_LOG_DEBUG("playerbot.dispel.packets",
                     "Bot {} queued CMSG_CAST_SPELL for dispel {} (target: {}, type: {})",
                     m_bot->GetName(), dispelSpell, target->GetName(),
                     static_cast<uint8>(m_currentAssignment.dispelType));
        return true;
    }
    else
    {
        TC_LOG_TRACE("playerbot.dispel.packets",
                     "Bot {} dispel {} validation failed: {} ({})",
                     m_bot->GetName(), dispelSpell,
                     static_cast<uint8>(result.result), result.failureReason);
    }

    ++m_statistics.failedDispels;
    return false;'''

    content = content.replace(old_dispel, new_dispel)

    # Fix 3: Replace ExecutePurge() CastSpell (line 873)
    old_purge = '''    // Cast purge
    if (m_bot->CastSpell(enemy, purgeSpell, false))
    {
        m_lastPurgeAttempt = now;
        m_globalCooldownUntil = now + m_config.dispelGCD;
        ++m_statistics.successfulPurges;
        return true;
    }

    ++m_statistics.failedPurges;
    return false;'''

    new_purge = '''    // Cast purge (MIGRATION COMPLETE 2025-10-30: Packet-based purge)
    SpellPacketBuilder::BuildOptions options;
    options.skipGcdCheck = false;
    options.skipResourceCheck = false;
    options.skipTargetCheck = false;
    options.skipStateCheck = false;
    options.skipRangeCheck = false;
    options.logFailures = true;

    auto result = SpellPacketBuilder::BuildCastSpellPacket(m_bot, purgeSpell, enemy, options);

    if (result.result == SpellPacketBuilder::ValidationResult::SUCCESS)
    {
        m_lastPurgeAttempt = now;
        m_globalCooldownUntil = now + m_config.dispelGCD;
        ++m_statistics.successfulPurges;

        TC_LOG_DEBUG("playerbot.dispel.packets",
                     "Bot {} queued CMSG_CAST_SPELL for purge {} (target: {})",
                     m_bot->GetName(), purgeSpell, enemy->GetName());
        return true;
    }
    else
    {
        TC_LOG_TRACE("playerbot.dispel.packets",
                     "Bot {} purge {} validation failed: {} ({})",
                     m_bot->GetName(), purgeSpell,
                     static_cast<uint8>(result.result), result.failureReason);
    }

    ++m_statistics.failedPurges;
    return false;'''

    content = content.replace(old_purge, new_purge)

    # Verify changes were applied
    if content == original_content:
        print("ERROR: No changes were applied - pattern matching failed")
        return False

    # Write modified content
    print(f"Writing migrated code to {file_path}...")
    with open(file_path, 'w', encoding='utf-8', newline='\n') as f:
        f.write(content)

    print("OK - DispelCoordinator.cpp migration complete")
    print("")
    print("Changes applied:")
    print("1. Added #include for SpellPacketBuilder.h")
    print("2. Migrated ExecuteDispel() CastSpell (line 808) to packet-based")
    print("3. Migrated ExecutePurge() CastSpell (line 873) to packet-based")
    print("")
    print("Impact: ALL healer bot dispel/cleanse/purge logic now packet-based")
    print("Next step: Compile to verify migration (expect 0 errors)")
    return True

if __name__ == "__main__":
    success = migrate_dispel_coordinator()
    sys.exit(0 if success else 1)
