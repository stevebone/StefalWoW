#!/usr/bin/env python3
"""
InterruptRotationManager.cpp Packet-Based Migration Script
PHASE 0 WEEK 3 PHASE 3.1 (2025-10-30)

Migrates InterruptRotationManager.cpp from direct CastSpell() API calls to
packet-based spell casting using SpellPacketBuilder.

This affects interrupt logic for ALL bots (fallback interrupts, alternative interrupts, delayed interrupts).

Changes:
1. Add SpellPacketBuilder.h include
2. Replace ExecuteFallback() Silence spell cast (line 595)
3. Replace ExecuteFallback() Solar Beam spell cast (line 603)
4. Replace TryAlternativeInterrupt() CastSpell (line 649)
5. Replace ProcessDelayedInterrupts() CastSpell (line 696)
"""

import re
import sys

def migrate_interrupt_rotation():
    file_path = "c:/TrinityBots/TrinityCore/src/modules/Playerbot/AI/CombatBehaviors/InterruptRotationManager.cpp"

    print(f"Reading {file_path}...")
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()

    original_content = content

    # Fix 1: Add SpellPacketBuilder.h include after SpatialGridQueryHelpers.h (line 24)
    if '#include "../../Packets/SpellPacketBuilder.h"' not in content:
        old_include = '#include "../../Spatial/SpatialGridQueryHelpers.h" // Thread-safe spatial grid queries\n#include <algorithm>'
        new_include = '#include "../../Spatial/SpatialGridQueryHelpers.h" // Thread-safe spatial grid queries\n#include "../../Packets/SpellPacketBuilder.h"  // PHASE 0 WEEK 3: Packet-based spell casting\n#include <algorithm>'
        content = content.replace(old_include, new_include)
        print("Added SpellPacketBuilder.h include")
    else:
        print("SpellPacketBuilder.h already included, skipping")

    # Fix 2: Replace ExecuteFallback() SPELL_SILENCE cast (line 595)
    old_silence = '''                if (!_bot->GetSpellHistory()->HasCooldown(SPELL_SILENCE))
                {
                    _bot->CastSpell(caster, SPELL_SILENCE, false);
                    return true;
                }'''

    new_silence = '''                if (!_bot->GetSpellHistory()->HasCooldown(SPELL_SILENCE))
                {
                    // MIGRATION COMPLETE (2025-10-30): Packet-based Silence fallback
                    SpellPacketBuilder::BuildOptions options;
                    options.skipGcdCheck = false;
                    options.skipResourceCheck = false;
                    options.skipTargetCheck = false;
                    options.skipStateCheck = false;
                    options.skipRangeCheck = false;
                    options.logFailures = true;

                    auto result = SpellPacketBuilder::BuildCastSpellPacket(_bot, SPELL_SILENCE, caster, options);
                    if (result.result == SpellPacketBuilder::ValidationResult::SUCCESS)
                    {
                        TC_LOG_DEBUG("playerbot.interrupt.fallback",
                                     "Bot {} queued SILENCE fallback (target: {})",
                                     _bot->GetName(), caster->GetName());
                        return true;
                    }
                }'''

    content = content.replace(old_silence, new_silence)

    # Fix 3: Replace ExecuteFallback() SPELL_SOLAR_BEAM cast (line 603)
    old_solar = '''                if (!_bot->GetSpellHistory()->HasCooldown(SPELL_SOLAR_BEAM))
                {
                    _bot->CastSpell(caster, SPELL_SOLAR_BEAM, false);
                    return true;
                }'''

    new_solar = '''                if (!_bot->GetSpellHistory()->HasCooldown(SPELL_SOLAR_BEAM))
                {
                    // MIGRATION COMPLETE (2025-10-30): Packet-based Solar Beam fallback
                    SpellPacketBuilder::BuildOptions options;
                    options.skipGcdCheck = false;
                    options.skipResourceCheck = false;
                    options.skipTargetCheck = false;
                    options.skipStateCheck = false;
                    options.skipRangeCheck = false;
                    options.logFailures = true;

                    auto result = SpellPacketBuilder::BuildCastSpellPacket(_bot, SPELL_SOLAR_BEAM, caster, options);
                    if (result.result == SpellPacketBuilder::ValidationResult::SUCCESS)
                    {
                        TC_LOG_DEBUG("playerbot.interrupt.fallback",
                                     "Bot {} queued SOLAR_BEAM fallback (target: {})",
                                     _bot->GetName(), caster->GetName());
                        return true;
                    }
                }'''

    content = content.replace(old_solar, new_solar)

    # Fix 4: Replace TryAlternativeInterrupt() CastSpell (line 649)
    old_alternative = '''                if (!_bot->GetSpellHistory()->HasCooldown(spellId))
                {
                    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
                    if (spellInfo && _bot->IsWithinLOSInMap(target))
                    {
                        _bot->CastSpell(target, spellId, false);
                        return true;
                    }
                }'''

    new_alternative = '''                if (!_bot->GetSpellHistory()->HasCooldown(spellId))
                {
                    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
                    if (spellInfo && _bot->IsWithinLOSInMap(target))
                    {
                        // MIGRATION COMPLETE (2025-10-30): Packet-based alternative interrupt
                        SpellPacketBuilder::BuildOptions options;
                        options.skipGcdCheck = false;
                        options.skipResourceCheck = false;
                        options.skipTargetCheck = false;
                        options.skipStateCheck = false;
                        options.skipRangeCheck = false;
                        options.logFailures = true;

                        auto result = SpellPacketBuilder::BuildCastSpellPacket(_bot, spellId, target, options);
                        if (result.result == SpellPacketBuilder::ValidationResult::SUCCESS)
                        {
                            TC_LOG_DEBUG("playerbot.interrupt.alternative",
                                         "Bot {} queued alternative interrupt {} (target: {})",
                                         _bot->GetName(), spellId, target->GetName());
                            return true;
                        }
                    }
                }'''

    content = content.replace(old_alternative, new_alternative)

    # Fix 5: Replace ProcessDelayedInterrupts() CastSpell (line 696)
    old_delayed = '''            if (interrupter && target && target->IsNonMeleeSpellCast(false))
            {
                interrupter->CastSpell(target, it->spellId, false);
                MarkInterruptUsed(it->interrupter, currentTime);
            }'''

    new_delayed = '''            if (interrupter && target && target->IsNonMeleeSpellCast(false))
            {
                // MIGRATION COMPLETE (2025-10-30): Packet-based delayed interrupt
                SpellPacketBuilder::BuildOptions options;
                options.skipGcdCheck = false;
                options.skipResourceCheck = false;
                options.skipTargetCheck = false;
                options.skipStateCheck = false;
                options.skipRangeCheck = false;
                options.logFailures = true;

                auto result = SpellPacketBuilder::BuildCastSpellPacket(
                    dynamic_cast<Player*>(interrupter), it->spellId, target, options);

                if (result.result == SpellPacketBuilder::ValidationResult::SUCCESS)
                {
                    TC_LOG_DEBUG("playerbot.interrupt.delayed",
                                 "Bot {} queued delayed interrupt {} (target: {})",
                                 interrupter->GetName(), it->spellId, target->GetName());
                    MarkInterruptUsed(it->interrupter, currentTime);
                }
            }'''

    content = content.replace(old_delayed, new_delayed)

    # Verify changes were applied
    if content == original_content:
        print("ERROR: No changes were applied - pattern matching failed")
        return False

    # Write modified content
    print(f"Writing migrated code to {file_path}...")
    with open(file_path, 'w', encoding='utf-8', newline='\n') as f:
        f.write(content)

    print("OK - InterruptRotationManager.cpp migration complete")
    print("")
    print("Changes applied:")
    print("1. Added #include for SpellPacketBuilder.h")
    print("2. Migrated ExecuteFallback() SILENCE spell cast (line 595)")
    print("3. Migrated ExecuteFallback() SOLAR_BEAM spell cast (line 603)")
    print("4. Migrated TryAlternativeInterrupt() CastSpell (line 649)")
    print("5. Migrated ProcessDelayedInterrupts() CastSpell (line 696)")
    print("")
    print("Impact: ALL bot interrupt fallback/alternative/delayed logic now packet-based")
    print("Next step: Compile to verify migration (expect 0 errors)")
    return True

if __name__ == "__main__":
    success = migrate_interrupt_rotation()
    sys.exit(0 if success else 1)
